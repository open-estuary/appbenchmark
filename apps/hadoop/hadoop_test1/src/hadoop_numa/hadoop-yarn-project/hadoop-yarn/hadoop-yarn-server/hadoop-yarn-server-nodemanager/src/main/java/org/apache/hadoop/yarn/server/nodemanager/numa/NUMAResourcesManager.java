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
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.service.CompositeService;
import org.apache.hadoop.util.Shell.ShellCommandExecutor;
import org.apache.hadoop.yarn.api.records.ContainerId;
import org.apache.hadoop.yarn.api.records.Resource;
import org.apache.hadoop.yarn.conf.YarnConfiguration;
import org.apache.hadoop.yarn.exceptions.YarnRuntimeException;

import com.google.common.annotations.VisibleForTesting;

/**
 * NUMA Resources Manager reads the NUMA topology and assigns NUMA nodes to the
 * containers.
 */
public class NUMAResourcesManager extends CompositeService {

  private static final Log LOG = LogFactory.getLog(NUMAResourcesManager.class);

  // Regex to find node ids, Ex: 'available: 2 nodes (0-1)'
  private static final String NUMA_NODEIDS_REGEX =
      "available:\\s*[0-9]+\\s*nodes\\s*\\(([0-9\\-,]*)\\)";

  // Regex to find node memory, Ex: 'node 0 size: 73717 MB'
  private static final String NUMA_NODE_MEMORY_REGEX =
      "node\\s*<NUMA-NODE>\\s*size:\\s*([0-9]*)\\s*([KMG]B)";

  // Regex to find node cpus, Ex: 'node 0 cpus: 0 2 4 6'
  private static final String NUMA_NODE_CPUS_REGEX =
      "node\\s*<NUMA-NODE>\\s*cpus:\\s*([0-9\\s]*)";

  private static final String GB = "GB";
  private static final String KB = "KB";
  private static final String NUMA_NODE = "<NUMA-NODE>";
  private static final String SPACE = "\\s";

  private List<NumaNode> numaNodesList = new ArrayList<NumaNode>();
  private int noOfNumaNodes;
  private int currentAssignNode;

  public NUMAResourcesManager() {
    super(NUMAResourcesManager.class.getName());
  }

  @Override
  public synchronized void serviceInit(Configuration conf) throws Exception {
    super.serviceInit(conf);
    if (conf.getBoolean(YarnConfiguration.NM_NUMA_AWARENESS_READ_TOPOLOGY,
        YarnConfiguration.DEFAULT_NM_NUMA_AWARENESS_READ_TOPOLOGY)) {
      LOG.info("Reading NUMA topology using 'numactl --hardware' command.");
      String cmdOutput = executeNGetCmdOutput();
      String[] outputLines = cmdOutput.split("\\n");
      Pattern pattern = Pattern.compile(NUMA_NODEIDS_REGEX);
      String nodeIdsStr = null;
      for (String line : outputLines) {
        Matcher matcher = pattern.matcher(line);
        if (matcher.find()) {
          nodeIdsStr = matcher.group(1);
          break;
        }
      }
      if (nodeIdsStr == null) {
        throw new YarnRuntimeException("Failed to get numa nodes from"
            + " 'numactl --hardware' output and output is:\n" + cmdOutput);
      }
      String[] nodeIdCommaSplits = nodeIdsStr.split("[,\\s]");
      for (String nodeIdRange : nodeIdCommaSplits) {
        if (nodeIdRange.contains("-")) {
          String[] beginNEnd = nodeIdRange.split("-");
          for (int nodeId = Integer.parseInt(beginNEnd[0]); nodeId <= Integer
              .parseInt(beginNEnd[1]); nodeId++) {
            long memory = parseMemory(outputLines, String.valueOf(nodeId));
            int cpus = parseCpus(outputLines, String.valueOf(nodeId));
            numaNodesList
                .add(new NumaNode(String.valueOf(nodeId), memory, cpus));
          }
        } else {
          long memory = parseMemory(outputLines, nodeIdRange);
          int cpus = parseCpus(outputLines, nodeIdRange);
          numaNodesList.add(new NumaNode(nodeIdRange, memory, cpus));
        }
      }
    } else {
      LOG.info("Reading NUMA topology using configurations.");
      Collection<String> nodeIds = conf
          .getStringCollection(YarnConfiguration.NM_NUMA_AWARENESS_NODE_IDS);
      for (String nodeId : nodeIds) {
        long mem = conf.getLong(
            "yarn.nodemanager.numa-awareness." + nodeId + ".memory", 0);
        int cpus = conf
            .getInt("yarn.nodemanager.numa-awareness." + nodeId + ".cpus", 0);
        numaNodesList.add(new NumaNode(nodeId, mem, cpus));
      }
    }
    if (numaNodesList.isEmpty()) {
      throw new YarnRuntimeException("There are no available NUMA nodes"
          + " for making containers NUMA aware.");
    }
    noOfNumaNodes = numaNodesList.size();
    LOG.info("Available numa nodes with capacities : " + numaNodesList);
  }

  @VisibleForTesting
  String executeNGetCmdOutput() throws YarnRuntimeException {
    String[] args = new String[] {"numactl", "--hardware"};
    ShellCommandExecutor shExec = new ShellCommandExecutor(args);
    try {
      shExec.execute();
    } catch (IOException e) {
      throw new YarnRuntimeException("Failed to read the numa configurations.",
          e);
    }
    String cmdOutput = shExec.getOutput();
    return cmdOutput;
  }

  private int parseCpus(String[] outputLines, String nodeId) {
    int cpus = 0;
    Pattern patternNodeCPUs = Pattern
        .compile(NUMA_NODE_CPUS_REGEX.replace(NUMA_NODE, nodeId));
    for (String line : outputLines) {
      Matcher matcherNodeCPUs = patternNodeCPUs.matcher(line);
      if (matcherNodeCPUs.find()) {
        String cpusStr = matcherNodeCPUs.group(1);
        cpus = cpusStr.split(SPACE).length;
        break;
      }
    }
    return cpus;
  }

  private long parseMemory(String[] outputLines, String nodeId) {
    long memory = 0;
    String units;
    Pattern patternNodeMem = Pattern
        .compile(NUMA_NODE_MEMORY_REGEX.replace(NUMA_NODE, nodeId));
    for (String line : outputLines) {
      Matcher matcherNodeMem = patternNodeMem.matcher(line);
      if (matcherNodeMem.find()) {
        try {
          memory = Long.parseLong(matcherNodeMem.group(1));
          units = matcherNodeMem.group(2);
          if (GB.equals(units)) {
            memory = memory * 1024;
          } else if (KB.equals(units)) {
            memory = memory / 1024;
          }
        } catch (Exception ex) {
          throw new YarnRuntimeException(
              "Failed to get memory for node:" + nodeId, ex);
        }
        break;
      }
    }
    return memory;
  }

  /**
   * Assigns the available NUMA nodes for the requested containerId with
   * resource in a round robin fashion.
   *
   * @param containerId the container ID
   * @param resource resource for the Numa Node
   * @return the assigned NUMA Node info or null if resources not available.
   */
  public synchronized AssignedNumaNodeInfo assignNumaNode(
      ContainerId containerId, Resource resource) {
    for (int index = 0; index < noOfNumaNodes; index++) {
      NumaNode numaNode = numaNodesList
          .get((currentAssignNode + index) % noOfNumaNodes);
      if (numaNode.isResourcesAvailable(resource)) {
        numaNode.assignResources(resource, containerId);
        LOG.info("Assigning NUMA node " + numaNode.getNodeId() + " for memory, "
            + numaNode.getNodeId() + " for cpus for the " + containerId);
        currentAssignNode = (currentAssignNode + index + 1) % noOfNumaNodes;
        return new AssignedNumaNodeInfo(numaNode.getNodeId(),
            numaNode.getNodeId());
      }
    }

    // If there is no single node matched for the container resource
    // Check the NUMA nodes for Memory resources
    StringBuilder memNodes = new StringBuilder();
    long memreq = resource.getMemorySize();
    for (NumaNode numaNode : numaNodesList) {
      memreq = numaNode.assignAvailableMemory(memreq, containerId);
      memNodes.append(numaNode.getNodeId());
      if (memreq == 0) {
        break;
      } else {
        memNodes.append(',');
      }
    }
    if (memreq != 0) {
      LOG.info("There is no available memory:" + resource.getMemorySize()
          + " in numa nodes for " + containerId);
      releaseNumaResources(containerId);
      return null;
    }

    // Check the NUMA nodes for CPU resources
    StringBuilder cpuNodes = new StringBuilder();
    int cpusreq = resource.getVirtualCores();
    for (int index = 0; index < noOfNumaNodes; index++) {
      NumaNode numaNode = numaNodesList
          .get((currentAssignNode + index) % noOfNumaNodes);
      cpusreq = numaNode.assignAvailableCpus(cpusreq, containerId);
      cpuNodes.append(numaNode.getNodeId());
      if (cpusreq == 0) {
        currentAssignNode = (currentAssignNode + index + 1) % noOfNumaNodes;
        break;
      } else {
        cpuNodes.append(',');
      }
    }

    if (cpusreq != 0) {
      LOG.info("There are no available cpus:" + resource.getVirtualCores()
          + " in numa nodes for " + containerId);
      releaseNumaResources(containerId);
      return null;
    }
    LOG.info("Assigning multiple NUMA nodes (" + memNodes.toString()
        + ") for memory, (" + cpuNodes.toString() + ") for cpus for "
        + containerId);
    return new AssignedNumaNodeInfo(memNodes.toString(), cpuNodes.toString());
  }

  /**
   * Release assigned NUMA resources for the container.
   *
   * @param containerId the container ID
   */
  public void releaseNumaResources(ContainerId containerId) {
    LOG.info("Releasing the assigned NUMA resources for " + containerId);
    for (NumaNode numaNode : numaNodesList) {
      numaNode.releaseResources(containerId);
    }
  }

  @VisibleForTesting
  Collection<NumaNode> getNumaNodesList() {
    return numaNodesList;
  }
}
