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

#include "qpid/sys/Dispatcher.h"

#include <assert.h>

namespace qpid {
namespace sys {

Dispatcher::Dispatcher(Poller::shared_ptr poller0) :
  poller(poller0) {
}

Dispatcher::~Dispatcher() {
}
    
void Dispatcher::run() {
    do {
        Poller::Event event = poller->wait();

        // Handle shutdown
        switch (event.type) {
        case Poller::SHUTDOWN:
            return;
            break;
        case Poller::INVALID:  // On any type of success or fail completion
            break;
        default:
          // This should be impossible
          assert(false);
        }
    } while (true);
}

}}
