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
package org.apache.qpid.test.unit.client.forwardall;

import junit.framework.JUnit4TestAdapter;
import org.junit.Test;
import org.junit.Before;
import org.junit.Assert;
import org.junit.After;
import org.apache.qpid.client.transport.TransportConnection;
import org.apache.qpid.client.vmbroker.AMQVMBrokerCreationException;

/**
 * Runs the Service's and Client parts of the test in the same process
 * as the broker
 */
public class CombinedTest
{

    @Before
    public void createVMBroker()
    {
        try
        {
            TransportConnection.createVMBroker(1);
        }
        catch (AMQVMBrokerCreationException e)
        {
            Assert.fail("Unable to create broker: " + e);
        }
    }

    @After
    public void stopVmBroker()
    {
        ServiceCreator.closeAll();
        TransportConnection.killVMBroker(1);
    }

    @Test
    public void forwardAll() throws Exception
    {
        int services = 2;
        ServiceCreator.start("vm://:1", services);

        System.out.println("Starting client...");

        new Client("vm://:1", services).shutdownWhenComplete();

        System.out.println("Completed successfully!");
    }

    public static junit.framework.Test suite()
    {
        return new JUnit4TestAdapter(CombinedTest.class);
    }
}
