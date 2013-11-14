/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include "qpid/linearstore/jrnl/enq_rec.h"

#include <cassert>
#include <cstring>
#include <iomanip>
#include "qpid/linearstore/jrnl/jexception.h"

namespace qpid {
namespace linearstore {
namespace journal {

enq_rec::enq_rec():
        jrec(), // superclass
        _xidp(0),
        _data(0),
        _buff(0)
{
    ::enq_hdr_init(&_enq_hdr, QLS_ENQ_MAGIC, QLS_JRNL_VERSION, 0, 0, 0, 0, false);
    ::rec_tail_copy(&_enq_tail, &_enq_hdr._rhdr, 0);
}

enq_rec::~enq_rec()
{
    clean();
}

void
enq_rec::reset(const uint64_t serial, const uint64_t rid, const void* const dbuf, const std::size_t dlen,
        const void* const xidp, const std::size_t xidlen, const bool transient, const bool external)
{
    _enq_hdr._rhdr._serial = serial;
    _enq_hdr._rhdr._rid = rid;
    ::set_enq_transient(&_enq_hdr, transient);
    ::set_enq_external(&_enq_hdr, external);
    _enq_hdr._xidsize = xidlen;
    _enq_hdr._dsize = dlen;
    _xidp = xidp;
    _data = dbuf;
    _buff = 0;
    _enq_tail._serial = serial;
    _enq_tail._rid = rid;
}

uint32_t
enq_rec::encode(void* wptr, uint32_t rec_offs_dblks, uint32_t max_size_dblks)
{
    assert(wptr != 0);
    assert(max_size_dblks > 0);
    if (_xidp == 0)
        assert(_enq_hdr._xidsize == 0);

    std::size_t rec_offs = rec_offs_dblks * QLS_DBLK_SIZE_BYTES;
    std::size_t rem = max_size_dblks * QLS_DBLK_SIZE_BYTES;
    std::size_t wr_cnt = 0;
    if (rec_offs_dblks) // Continuation of split data record (over 2 or more pages)
    {
        if (size_dblks(rec_size()) - rec_offs_dblks > max_size_dblks) // Further split required
        {
            rec_offs -= sizeof(_enq_hdr);
            std::size_t wsize = _enq_hdr._xidsize > rec_offs ? _enq_hdr._xidsize - rec_offs : 0;
            std::size_t wsize2 = wsize;
            if (wsize)
            {
                if (wsize > rem)
                    wsize = rem;
                std::memcpy(wptr, (const char*)_xidp + rec_offs, wsize);
                wr_cnt = wsize;
                rem -= wsize;
            }
            rec_offs -= _enq_hdr._xidsize - wsize2;
            if (rem && !::is_enq_external(&_enq_hdr))
            {
                wsize = _enq_hdr._dsize > rec_offs ? _enq_hdr._dsize - rec_offs : 0;
                wsize2 = wsize;
                if (wsize)
                {
                    if (wsize > rem)
                        wsize = rem;
                    std::memcpy((char*)wptr + wr_cnt, (const char*)_data + rec_offs, wsize);
                    wr_cnt += wsize;
                    rem -= wsize;
                }
                rec_offs -= _enq_hdr._dsize - wsize2;
            }
            if (rem)
            {
                wsize = sizeof(_enq_tail) > rec_offs ? sizeof(_enq_tail) - rec_offs : 0;
                wsize2 = wsize;
                if (wsize)
                {
                    if (wsize > rem)
                        wsize = rem;
                    std::memcpy((char*)wptr + wr_cnt, (char*)&_enq_tail + rec_offs, wsize);
                    wr_cnt += wsize;
                    rem -= wsize;
                }
                rec_offs -= sizeof(_enq_tail) - wsize2;
            }
            assert(rem == 0);
            assert(rec_offs == 0);
        }
        else // No further split required
        {
            rec_offs -= sizeof(_enq_hdr);
            std::size_t wsize = _enq_hdr._xidsize > rec_offs ? _enq_hdr._xidsize - rec_offs : 0;
            if (wsize)
            {
                std::memcpy(wptr, (const char*)_xidp + rec_offs, wsize);
                wr_cnt += wsize;
            }
            rec_offs -= _enq_hdr._xidsize - wsize;
            wsize = _enq_hdr._dsize > rec_offs ? _enq_hdr._dsize - rec_offs : 0;
            if (wsize && !::is_enq_external(&_enq_hdr))
            {
                std::memcpy((char*)wptr + wr_cnt, (const char*)_data + rec_offs, wsize);
                wr_cnt += wsize;
            }
            rec_offs -= _enq_hdr._dsize - wsize;
            wsize = sizeof(_enq_tail) > rec_offs ? sizeof(_enq_tail) - rec_offs : 0;
            if (wsize)
            {
                std::memcpy((char*)wptr + wr_cnt, (char*)&_enq_tail + rec_offs, wsize);
                wr_cnt += wsize;
#ifdef QLS_CLEAN
                std::size_t rec_offs = rec_offs_dblks * QLS_DBLK_SIZE_BYTES;
                std::size_t dblk_rec_size = size_dblks(rec_size() - rec_offs) * QLS_DBLK_SIZE_BYTES;
                std::memset((char*)wptr + wr_cnt, QLS_CLEAN_CHAR, dblk_rec_size - wr_cnt);
#endif
            }
            rec_offs -= sizeof(_enq_tail) - wsize;
            assert(rec_offs == 0);
        }
    }
    else // Start at beginning of data record
    {
        // Assumption: the header will always fit into the first dblk
        std::memcpy(wptr, (void*)&_enq_hdr, sizeof(_enq_hdr));
        wr_cnt = sizeof(_enq_hdr);
        if (size_dblks(rec_size()) > max_size_dblks) // Split required
        {
            std::size_t wsize;
            rem -= sizeof(_enq_hdr);
            if (rem)
            {
                wsize = rem >= _enq_hdr._xidsize ? _enq_hdr._xidsize : rem;
                std::memcpy((char*)wptr + wr_cnt,  _xidp, wsize);
                wr_cnt += wsize;
                rem -= wsize;
            }
            if (rem && !::is_enq_external(&_enq_hdr))
            {
                wsize = rem >= _enq_hdr._dsize ? _enq_hdr._dsize : rem;
                std::memcpy((char*)wptr + wr_cnt, _data, wsize);
                wr_cnt += wsize;
                rem -= wsize;
            }
            if (rem)
            {
                wsize = rem >= sizeof(_enq_tail) ? sizeof(_enq_tail) : rem;
                std::memcpy((char*)wptr + wr_cnt, (void*)&_enq_tail, wsize);
                wr_cnt += wsize;
                rem -= wsize;
            }
            assert(rem == 0);
        }
        else // No split required
        {
            if (_enq_hdr._xidsize)
            {
                std::memcpy((char*)wptr + wr_cnt, _xidp, _enq_hdr._xidsize);
                wr_cnt += _enq_hdr._xidsize;
            }
            if (!::is_enq_external(&_enq_hdr))
            {
                std::memcpy((char*)wptr + wr_cnt, _data, _enq_hdr._dsize);
                wr_cnt += _enq_hdr._dsize;
            }
            std::memcpy((char*)wptr + wr_cnt, (void*)&_enq_tail, sizeof(_enq_tail));
            wr_cnt += sizeof(_enq_tail);
#ifdef QLS_CLEAN
            std::size_t dblk_rec_size = size_dblks(rec_size()) * QLS_DBLK_SIZE_BYTES;
            std::memset((char*)wptr + wr_cnt, QLS_CLEAN_CHAR, dblk_rec_size - wr_cnt);
#endif
        }
    }
    return size_dblks(wr_cnt);
}

bool
enq_rec::decode(::rec_hdr_t& h, std::ifstream* ifsp, std::size_t& rec_offs)
{
    uint32_t checksum = 0UL; // TODO: Add checksum math
    if (rec_offs == 0)
    {
        // Read header, allocate (if req'd) for xid
        //_enq_hdr.hdr_copy(h);
        ::rec_hdr_copy(&_enq_hdr._rhdr, &h);
        ifsp->read((char*)&_enq_hdr._xidsize, sizeof(_enq_hdr._xidsize));
        ifsp->read((char*)&_enq_hdr._dsize, sizeof(_enq_hdr._dsize));
        rec_offs = sizeof(_enq_hdr);
        if (_enq_hdr._xidsize > 0)
        {
            _buff = std::malloc(_enq_hdr._xidsize);
            MALLOC_CHK(_buff, "_buff", "enq_rec", "rcv_decode");
        }
    }
    if (rec_offs < sizeof(_enq_hdr) + _enq_hdr._xidsize)
    {
        // Read xid (or continue reading xid)
        std::size_t offs = rec_offs - sizeof(_enq_hdr);
        ifsp->read((char*)_buff + offs, _enq_hdr._xidsize - offs);
        std::size_t size_read = ifsp->gcount();
        rec_offs += size_read;
        if (size_read < _enq_hdr._xidsize - offs)
        {
            assert(ifsp->eof());
            // As we may have read past eof, turn off fail bit
            ifsp->clear(ifsp->rdstate()&(~std::ifstream::failbit));
            assert(!ifsp->fail() && !ifsp->bad());
            return false;
        }
    }
    if (!::is_enq_external(&_enq_hdr))
    {
        if (rec_offs < sizeof(_enq_hdr) + _enq_hdr._xidsize +  _enq_hdr._dsize)
        {
            // Ignore data (or continue ignoring data)
            std::size_t offs = rec_offs - sizeof(_enq_hdr) - _enq_hdr._xidsize;
            ifsp->ignore(_enq_hdr._dsize - offs);
            std::size_t size_read = ifsp->gcount();
            rec_offs += size_read;
            if (size_read < _enq_hdr._dsize - offs)
            {
                assert(ifsp->eof());
                // As we may have read past eof, turn off fail bit
                ifsp->clear(ifsp->rdstate()&(~std::ifstream::failbit));
                assert(!ifsp->fail() && !ifsp->bad());
                return false;
            }
        }
    }
    if (rec_offs < sizeof(_enq_hdr) + _enq_hdr._xidsize +
            (::is_enq_external(&_enq_hdr) ? 0 : _enq_hdr._dsize) + sizeof(rec_tail_t))
    {
        // Read tail (or continue reading tail)
        std::size_t offs = rec_offs - sizeof(_enq_hdr) - _enq_hdr._xidsize;
        if (!::is_enq_external(&_enq_hdr))
            offs -= _enq_hdr._dsize;
        ifsp->read((char*)&_enq_tail + offs, sizeof(rec_tail_t) - offs);
        std::size_t size_read = ifsp->gcount();
        rec_offs += size_read;
        if (size_read < sizeof(rec_tail_t) - offs)
        {
            assert(ifsp->eof());
            // As we may have read past eof, turn off fail bit
            ifsp->clear(ifsp->rdstate()&(~std::ifstream::failbit));
            assert(!ifsp->fail() && !ifsp->bad());
            return false;
        }
    }
    ifsp->ignore(rec_size_dblks() * QLS_DBLK_SIZE_BYTES - rec_size());
    assert(!ifsp->fail() && !ifsp->bad());
    int res = ::rec_tail_check(&_enq_tail, &_enq_hdr._rhdr, checksum);
    if (res != 0) {
        std::stringstream oss;
        switch (res) {
          case 1: oss << std::hex << "Magic: expected 0x" << ~_enq_hdr._rhdr._magic << "; found 0x" << _enq_tail._xmagic; break;
          case 2: oss << std::hex << "Serial: expected 0x" << _enq_hdr._rhdr._serial << "; found 0x" << _enq_tail._serial; break;
          case 3: oss << std::hex << "Record Id: expected 0x" << _enq_hdr._rhdr._rid << "; found 0x" << _enq_tail._rid; break;
          case 4: oss << std::hex << "Checksum: expected 0x" << checksum << "; found 0x" << _enq_tail._checksum; break;
          default: oss << "Unknown error " << res;
        }
        throw jexception(jerrno::JERR_JREC_BADRECTAIL, oss.str(), "enq_rec", "decode"); // TODO: Don't throw exception, log info
    }
    return true;
}

std::size_t
enq_rec::get_xid(void** const xidpp)
{
    if (!_buff || !_enq_hdr._xidsize)
    {
        *xidpp = 0;
        return 0;
    }
    *xidpp = _buff;
    return _enq_hdr._xidsize;
}

std::size_t
enq_rec::get_data(void** const datapp)
{
    if (!_buff)
    {
        *datapp = 0;
        return 0;
    }
    if (::is_enq_external(&_enq_hdr))
        *datapp = 0;
    else
        *datapp = (void*)((char*)_buff + _enq_hdr._xidsize);
    return _enq_hdr._dsize;
}

std::string&
enq_rec::str(std::string& str) const
{
    std::ostringstream oss;
    oss << "enq_rec: m=" << _enq_hdr._rhdr._magic;
    oss << " v=" << (int)_enq_hdr._rhdr._version;
    oss << " rid=" << _enq_hdr._rhdr._rid;
    if (_xidp)
        oss << " xid=\"" << _xidp << "\"";
    oss << " len=" << _enq_hdr._dsize;
    str.append(oss.str());
    return str;
}

std::size_t
enq_rec::rec_size() const
{
    return rec_size(_enq_hdr._xidsize, _enq_hdr._dsize, ::is_enq_external(&_enq_hdr));
}

std::size_t
enq_rec::rec_size(const std::size_t xidsize, const std::size_t dsize, const bool external)
{
    if (external)
        return sizeof(enq_hdr_t) + xidsize + sizeof(rec_tail_t);
    return sizeof(enq_hdr_t) + xidsize + dsize + sizeof(rec_tail_t);
}

void
enq_rec::clean()
{
    // clean up allocated memory here
}

}}}
