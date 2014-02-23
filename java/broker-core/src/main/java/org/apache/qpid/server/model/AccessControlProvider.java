/*
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
package org.apache.qpid.server.model;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.UUID;

import org.apache.qpid.server.security.AccessControl;

public interface AccessControlProvider<X extends AccessControlProvider<X>> extends ConfiguredObject<X>
{
    public static final String DESCRIPTION = "description";
    public static final String STATE = "state";
    public static final String DURABLE = "durable";
    public static final String LIFETIME_POLICY = "lifetimePolicy";
    public static final String TIME_TO_LIVE = "timeToLive";
    public static final String CREATED = "created";
    public static final String UPDATED = "updated";
    public static final String TYPE = "type";

    //retrieve the underlying AccessControl object
    AccessControl getAccessControl();
}
