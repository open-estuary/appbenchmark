/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.hadoop.yarn.server.nodemanager.numa;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.yarn.api.records.ContainerId;
import org.apache.hadoop.yarn.api.records.Resource;
import org.apache.hadoop.yarn.conf.YarnConfiguration;
import org.apache.hadoop.yarn.exceptions.YarnRuntimeException;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

/**
 * Test class for NUMAResourcesManager.
 */
public class TestNUMAResourcesManager {

  private Configuration conf;
  private NUMAResourcesManager numaResourcesManager;

  @Before
  public void setUp() throws IOException {
    conf = new YarnConfiguration();
    numaResourcesManager = new NUMAResourcesManager();
  }

  @Test
  public void testReadNumaTopologyFromConfigurations() throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    Collection<NumaNode> nodesList = numaResourcesManager.getNumaNodesList();
    Collection<NumaNode> expectedNodesList = getExpectedNumaNodesList();
    Assert.assertEquals(expectedNodesList, nodesList);
  }

  @Test
  public void testReadNumaTopologyFromCmdOutput() throws Exception {
    conf.setBoolean(YarnConfiguration.NM_NUMA_AWARENESS_READ_TOPOLOGY, true);
    String cmdOutput = "available: 2 nodes (0-1)\n\t"
        + "node 0 cpus: 0 2 4 6\n\t"
        + "node 0 size: 73717 MB\n\t"
        + "node 0 free: 17272 MB\n\t"
        + "node 1 cpus: 1 3 5 7\n\t"
        + "node 1 size: 73727 MB\n\t"
        + "node 1 free: 10699 MB\n\t"
        + "node distances:\n\t"
        + "node 0 1\n\t"
        + "0: 10 20\n\t"
        + "1: 20 10";
    numaResourcesManager = new NUMAResourcesManager() {
      @Override
      String executeNGetCmdOutput() throws YarnRuntimeException {
        return cmdOutput;
      }
    };
    numaResourcesManager.serviceInit(conf);
    Collection<NumaNode> nodesList = numaResourcesManager.getNumaNodesList();
    Collection<NumaNode> expectedNodesList = getExpectedNumaNodesList();
    Assert.assertEquals(expectedNodesList, nodesList);
  }

  @Test
  public void testAssignNumaNode() throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(2048, 2));
    Assert.assertEquals("0", nodeInfo.getMemNode());
    Assert.assertEquals("0", nodeInfo.getCpuNode());
  }

  @Test
  public void testAssignNumaNodeWithRoundRobinFashionAssignment()
      throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo1 = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(2048, 2));
    Assert.assertEquals("0", nodeInfo1.getMemNode());
    Assert.assertEquals("0", nodeInfo1.getCpuNode());

    AssignedNumaNodeInfo nodeInfo2 = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000002"),
        Resource.newInstance(2048, 2));
    Assert.assertEquals("1", nodeInfo2.getMemNode());
    Assert.assertEquals("1", nodeInfo2.getCpuNode());

    AssignedNumaNodeInfo nodeInfo3 = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000003"),
        Resource.newInstance(2048, 2));
    Assert.assertEquals("0", nodeInfo3.getMemNode());
    Assert.assertEquals("0", nodeInfo3.getCpuNode());

    AssignedNumaNodeInfo nodeInfo4 = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000003"),
        Resource.newInstance(2048, 2));
    Assert.assertEquals("1", nodeInfo4.getMemNode());
    Assert.assertEquals("1", nodeInfo4.getCpuNode());
  }

  @Test
  public void testAssignNumaNodeWithMultipleNodesForMemory() throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(102400, 2));
    Assert.assertEquals("0,1", nodeInfo.getMemNode());
    Assert.assertEquals("0", nodeInfo.getCpuNode());
  }

  @Test
  public void testAssignNumaNodeWithMultipleNodesForCpus() throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(2048, 6));
    Assert.assertEquals("0", nodeInfo.getMemNode());
    Assert.assertEquals("0,1", nodeInfo.getCpuNode());
  }

  @Test
  public void testAssignNumaNodeWhenNoNumaMemResourcesAvailable()
      throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(2048000, 6));
    Assert.assertNull("Should not assign numa nodes when there"
        + " are no sufficient memory resources available.", nodeInfo);
  }

  @Test
  public void testAssignNumaNodeWhenNoNumaCpuResourcesAvailable()
      throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(2048, 600));
    Assert.assertNull("Should not assign numa nodes when there"
        + " are no sufficient cpu resources available.", nodeInfo);
  }

  @Test
  public void testReleaseNumaResourcess() throws Exception {
    setNumaTopologyConfigs();
    numaResourcesManager.serviceInit(conf);
    AssignedNumaNodeInfo nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000001"),
        Resource.newInstance(2048, 8));
    Assert.assertEquals("0", nodeInfo.getMemNode());
    Assert.assertEquals("0,1", nodeInfo.getCpuNode());

    // Request the resource when all cpu nodes occupied
    nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000002"),
        Resource.newInstance(2048, 4));
    Assert.assertNull("Should not assign numa nodes when there"
        + " are no sufficient cpu resources available.", nodeInfo);

    // Release the resources
    numaResourcesManager.releaseNumaResources(
        ContainerId.fromString("container_1481156246874_0001_01_000001"));
    // Request the resources
    nodeInfo = numaResourcesManager.assignNumaNode(
        ContainerId.fromString("container_1481156246874_0001_01_000003"),
        Resource.newInstance(1024, 2));
    Assert.assertEquals("0", nodeInfo.getMemNode());
    Assert.assertEquals("0", nodeInfo.getCpuNode());
  }

  private void setNumaTopologyConfigs() {
    conf.set(YarnConfiguration.NM_NUMA_AWARENESS_NODE_IDS, "0,1");
    conf.set("yarn.nodemanager.numa-awareness.0.memory", "73717");
    conf.set("yarn.nodemanager.numa-awareness.0.cpus", "4");
    conf.set("yarn.nodemanager.numa-awareness.1.memory", "73727");
    conf.set("yarn.nodemanager.numa-awareness.1.cpus", "4");
  }

  private Collection<NumaNode> getExpectedNumaNodesList() {
    Collection<NumaNode> expectedNodesList = new ArrayList<>(2);
    expectedNodesList.add(new NumaNode("0", 73717, 4));
    expectedNodesList.add(new NumaNode("1", 73727, 4));
    return expectedNodesList;
  }
}
