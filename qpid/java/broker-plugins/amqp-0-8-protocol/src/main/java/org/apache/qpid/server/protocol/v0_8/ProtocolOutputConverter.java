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

/*
 * This file is auto-generated by Qpid Gentools v.0.1 - do not modify.
 * Supported AMQP versions:
 *   8-0
 */
package org.apache.qpid.server.protocol.v0_8;

import org.apache.qpid.framing.AMQDataBlock;
import org.apache.qpid.framing.AMQShortString;
import org.apache.qpid.framing.ContentHeaderBody;
import org.apache.qpid.framing.MessagePublishInfo;
import org.apache.qpid.server.message.InstanceProperties;
import org.apache.qpid.server.message.MessageContentSource;
import org.apache.qpid.server.message.ServerMessage;

public interface ProtocolOutputConverter
{
    void confirmConsumerAutoClose(int channelId, AMQShortString consumerTag);

    long writeDeliver(final ServerMessage msg,
                      final InstanceProperties props, int channelId,
                      long deliveryTag,
                      AMQShortString consumerTag);

    long writeGetOk(final ServerMessage msg,
                    final InstanceProperties props,
                    int channelId,
                    long deliveryTag,
                    int queueSize);

    void writeReturn(MessagePublishInfo messagePublishInfo, ContentHeaderBody header, MessageContentSource msgContent,  int channelId, int replyCode, AMQShortString replyText);

    void writeFrame(AMQDataBlock block);
}