#coding=utf-8
import sys,os
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import Element
import xml.dom.minidom 

def create_pnode(dom):
 element= dom.createElement('property')
 return element

def create_nnode(name,value,dom):
 element=dom.createElement('name')
 element.appendChild(dom.createTextNode(value))
 return element

def create_vnode(name,value,dom):
 element=dom.createElement('value')
 element.appendChild(dom.createTextNode(value))
 return element

def create_kv(nametext,valuetext,tree):
 root=tree.documentElement
 property=create_pnode(tree)
 name=create_nnode("name",nametext,tree)
 value=create_vnode("value",valuetext,tree)
 root.appendChild(property)
 property.appendChild(name)
 property.appendChild(value)
 return root

def analysis_xml(component,doc):
 confs=component.getElementsByTagName("configuration-definition")
 for conf in confs:
  names=conf.getElementsByTagName("name")
  values=conf.getElementsByTagName("value")
  name=names[0].childNodes[0].nodeValue
  value=values[0].childNodes[0].nodeValue
  create_kv(name,value,doc)

def write_xml(XML_DIR,doc):
 f = open(XML_DIR, "w")
 f.write(doc.toprettyxml(indent = "\t", newl = "\n", encoding = "utf-8"))
 f.close()


def main():	
 dom=xml.dom.minidom.parse(sys.argv[0].replace('updatexml.py','conf.xml'))
 root=dom.documentElement
 components=root.getElementsByTagName("component")
 for component in components:
  sites=component.getElementsByTagName("xml")
  site=sites[0].childNodes[0].nodeValue 
  if site == 'HDFS':
   XML_DIR=os.path.join(sys.argv[1],"etc/hadoop/hdfs-site.xml")
   doc=xml.dom.minidom.parse(XML_DIR)
#   create_kv("ha.zookeeper.quorum",os.environ['zk_address'],doc)
   create_kv("dfs.namenode.name.dir",'file:' + os.environ['dfs_namenode_name_dir'],doc)
   create_kv("dfs.datanode.data.dir",'file:' + os.environ['dfs_datanode_data_dir'],doc)
#   create_kv("dfs.ha.namenodes.hacluster",os.environ['dfs_ha_namenode_id_1']+","+os.environ['dfs_ha_namenode_id_2'],doc)
#   create_kv("dfs.namenode.rpc-address.hacluster.nn1",os.environ['dfs_namenode_rpc_address_hacluster_nn1'],doc)
#   create_kv("dfs.namenode.rpc-address.hacluster.nn2",os.environ['dfs_namenode_rpc_address_hacluster_nn2'],doc)
#   create_kv("dfs.namenode.http-address.hacluster.nn1",os.environ['dfs_namenode_http_address_hacluster_nn1'],doc)
#   create_kv("dfs.namenode.http-address.hacluster.nn2",os.environ['dfs_namenode_http_address_hacluster_nn2'],doc)
#   create_kv("dfs.namenode.shared.edits.dir",os.environ['dfs_namenode_shared_edits_dir'],doc)
#   create_kv("dfs.journalnode.edits.dir",os.environ['HADOOP_ROOT']+"/journalnode/journalDir",doc)
   analysis_xml(component,doc)
   write_xml(XML_DIR,doc)
  elif site == 'core':
   XML_DIR=os.path.join(sys.argv[1],"etc/hadoop/core-site.xml")
   doc=xml.dom.minidom.parse(XML_DIR)
   create_kv("hadoop.tmp.dir",'file:' + os.environ['hadoop_tmp_dir'],doc)
   analysis_xml(component,doc)
   write_xml(XML_DIR,doc)
  elif site =='yarn':
   XML_DIR=os.path.join(sys.argv[1],"etc/hadoop/yarn-site.xml")
   doc=xml.dom.minidom.parse(XML_DIR)
#   create_kv("yarn.resourcemanager.address.rm1",os.environ['yarn_resourcemanager_address_rm1'],doc)
#   create_kv("yarn.resourcemanager.admin.address.rm1",os.environ['yarn_resourcemanager_admin_address_rm1'],doc)
#   create_kv("yarn.resourcemanager.resource-tracker.address.rm1",os.environ['yarn_resourcemanager_resource_tracker_address_rm1'],doc)
#   create_kv("yarn.resourcemanager.scheduler.address.rm1",os.environ['yarn_resourcemanager_scheduler_address_rm1'],doc)
#   create_kv("yarn.resourcemanager.webapp.address.rm1",os.environ['yarn_resourcemanager_webapp_address_rm1'],doc)
#   create_kv("yarn.resourcemanager.address.rm2",os.environ['yarn_resourcemanager_address_rm2'],doc)
#   create_kv("yarn.resourcemanager.admin.address.rm2",os.environ['yarn_resourcemanager_admin_address_rm2'],doc)
#   create_kv("yarn.resourcemanager.resource-tracker.address.rm2",os.environ['yarn_resourcemanager_resource_tracker_address_rm2'],doc)
#   create_kv("yarn.resourcemanager.scheduler.address.rm2",os.environ['yarn_resourcemanager_scheduler_address_rm2'],doc)
#   create_kv("yarn.resourcemanager.webapp.address.rm2",os.environ['yarn_resourcemanager_webapp_address_rm2'],doc)
#   create_kv("yarn.nodemanager.resource.memory-mb",os.environ['NODEMANAGER_RESOURCE_MEMORY'],doc)
#   create_kv("yarn.nodemanager.local-dirs",os.environ['mapred_local_dir'],doc)
#   create_kv("yarn.nodemanager.local-dirs",os.environ['mapred_local_dir'],doc)
#   create_kv("yarn.nodemanager.log-dirs",os.environ['yarn_nodemanager_log_dirs'],doc)
#   create_kv("yarn.resourcemanager.zk-address",os.environ['zk_address'],doc)
#   create_kv("mapreduce.jobhistory.address",os.environ['mapreduce_jobhistory_address'],doc)
#   create_kv("mapreduce.jobhistory.webapp.address",os.environ['mapreduce_jobhistory_webapp_address'],doc)
#   if 'resourcemanager'==sys.argv[1]:
#    create_kv("yarn.resourcemanager.ha.id","rm"+sys.argv[2],doc)
#    create_kv("yarn.scheduler.maximum-allocation-vcores",os.environ['MAX_ALLOCATION_VCORES'],doc)
#    create_kv("yarn.scheduler.maximum-allocation-mb",os.environ['MAX_ALLOCATION_MEMORY'],doc)
#   if 'nodemanager'==sys.argv[1]:
#    create_kv("yarn.nodemanager.resource.memory-mb",os.environ['NODEMANAGER_RESOURCE_MEMORY'],doc)
#    create_kv("yarn.nodemanager.resource.cpu-vcores",os.environ['NODEMANAGER_RESOURCE_CPU'],doc)
   analysis_xml(component,doc)
   write_xml(XML_DIR,doc)
  elif site=='mapreduce':
   XML_DIR=os.path.join(sys.argv[1],"etc/hadoop/mapred-site.xml.template")
   doc=xml.dom.minidom.parse(XML_DIR)
   analysis_xml(component,doc)
   XML_DIR=os.path.join(sys.argv[1],"etc/hadoop/mapred-site.xml")
   write_xml(XML_DIR,doc)
		

if __name__=='__main__':
        main()

