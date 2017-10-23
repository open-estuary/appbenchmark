package src.main.java.cn.net.communion.core;

import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import src.main.java.cn.net.communion.entity.JobInfo;
import src.main.java.cn.net.communion.entity.KeyValue;
import src.main.java.cn.net.communion.helper.ArgParser;
import src.main.java.cn.net.communion.helper.FileHelper;
import src.main.java.cn.net.communion.main.App;

public class Generator {

	public void start(List<JobInfo> jobList) throws IOException {
		long sum = 0;
		long prepareTotalTime = 0;
		long runSqlTotalTime = 0;
		long startTime = 0;
		long endTime = 0;

		for (JobInfo job : jobList) {
			System.out.println("[job-" + job.getId() + "]" + job.getNum() + " start!");
			System.out.println("==============================================================");
			System.out.println("sql method:"+"replace");
			Map<String, String> tempMap = new HashMap<String, String>();
			List<KeyValue> detail = job.getDetail();
			List<String> sqlList = new ArrayList<String>();
			String filename = job.getFilename();
			if (filename == null) {
				for (int i = 0; i < job.getNum(); i++) {
					fillTempMap(detail, tempMap);
					FileHelper.save(null, job.getTable(), tempMap); 
					tempMap.clear();
				}	
			} else {
				FileWriter writer = new FileWriter("" + filename, true);
				if (filename.indexOf(".csv") > 0) {
					for (int i = 0; i < job.getNum(); i++) {
						fillTempMap(detail, tempMap);
						if (i == 0) {	
							FileHelper.save(writer, tempMap.keySet());
						}
						FileHelper.save(writer, tempMap.values());
						tempMap.clear();
					}
				} else {
					startTime =System.currentTimeMillis(); 
					for (int i = 0; i < job.getNum(); i++) {
						fillTempMap(detail, tempMap);
						FileHelper.save(writer,job.getTable(), tempMap );
						String insertSQL = FileHelper.getInsertSQL(job.getTable(), tempMap).toString();
						sqlList.add(insertSQL);
						tempMap.clear();  
					}
					//                	startTime =System.currentTimeMillis(); 
					//                	int threadNum=App.THREADNUM;
					//                	int split = job.getNum()/threadNum;
					//                	int left = job.getNum()-(split*threadNum);
					//                	CreateData[] create_sql=new CreateData[threadNum];
					//                	System.out.println("work thread: "+threadNum);
					//                	startTime =System.currentTimeMillis(); 
					//                	if(split!=0)
					//                	{
					//                    	for(int i=0;i<threadNum;i++)
					//                    	{
					//                    		CreateData rs = new CreateData(writer,detail,sqlList,job,i*split,i*split+split);
					//                    		create_sql[i] = rs;
					//                    		rs.start();
					//                    	}
					//                	}
					//                	CreateData lrs = null;
					//                	if(left!=0)
					//                	{
					//                		lrs = new CreateData(writer,detail,sqlList,job,threadNum*split,job.getNum());
					//                		lrs.start();
					//                	}
					//                	try
					//                	{
					//                		if(split!=0)
					//                		{
					//                    		for(int i=0;i<threadNum;i++)
					//                        	{
					//                    			create_sql[i].join();
					//                        	}
					//                		}
					//                		if(lrs!=null)
					//                			lrs.join();
					//                	} catch
					//                	(InterruptedException e) {
					//
					//                	e.printStackTrace();
					//
					//                	}
					//                	
					//                	endTime = System.currentTimeMillis(); 
					//                	prepareTotalTime+= endTime-startTime;
					//                	System.out.println("Prepare data success!");
					//                	System.out.println("Prepare Time: "+(endTime-startTime)+"ms");
					//                	System.out.println("--------------------------------------------------------------");
					endTime = System.currentTimeMillis(); 
					prepareTotalTime+= endTime-startTime;
					System.out.println("Prepare data success!");
					System.out.println("Prepare Time: "+(endTime-startTime)+"ms");
					System.out.println("--------------------------------------------------------------");
				}
				writer.close();
				if(ArgParser.DATABASEOP == "open")
				{
					System.out.println("waiting for replace data......");
					int threadNum=ArgParser.THREADNUM;
					int split = job.getNum()/threadNum;
					int left = job.getNum()-(split*threadNum);
					RunSQL[] thread_sql=new RunSQL[threadNum];
					System.out.println("work thread: "+threadNum);
					startTime =System.currentTimeMillis(); 
					if(split!=0)
					{
						for(int i=0;i<threadNum;i++)
						{
							RunSQL rs = new RunSQL(detail,job,split,writer,i*split,i*split+split,sqlList,ArgParser.IP,ArgParser.PASSWORD,ArgParser.PORT,ArgParser.DB);
							thread_sql[i] = rs;
							rs.start();
						}
					}
					RunSQL lrs = null;
					if(left!=0)
					{
						lrs = new RunSQL(detail,job,left,writer,threadNum*split,job.getNum(),sqlList,ArgParser.IP,ArgParser.PASSWORD,ArgParser.PORT,ArgParser.DB);
						lrs.start();
					}
					try
					{
						if(split!=0)
						{
							for(int i=0;i<threadNum;i++)
							{
								thread_sql[i].join();
							}
						}
						if(lrs!=null)
							lrs.join();
					} catch
					(InterruptedException e) {

						e.printStackTrace();

					}
					endTime = System.currentTimeMillis(); 
					runSqlTotalTime+= endTime-startTime;
					System.out.println("replace data success!");
					System.out.println("replace Time: "+(endTime-startTime)+"ms");
				}
			}
			System.out.println("[job-" + job.getId() + "]" + job.getNum() + " data had writen to " + (filename != null ? "file" : "console") );
			sum += job.getNum();
		}
		if(ArgParser.DATABASEOP == "open")
		{
			System.out.println("==============================================================");
			System.out.println("generate data successfully, total: " + sum);
			System.out.println("Total run time£º "+(prepareTotalTime+runSqlTotalTime)+"ms");
			System.out.println("Total prepare data time£º "+(prepareTotalTime)+"ms");
			System.out.println("Total run sql time£º "+(runSqlTotalTime)+"ms");
			System.out.println("OPS: " + ((float)sum/(runSqlTotalTime))*1000+"/s" );
		}
	}

	Map<String, String>  fillTempMap(List<KeyValue> detail, Map<String, String> tempMap) {
		for (KeyValue kv : detail) {
			tempMap.put(kv.key, Dispatcher.checkGrammer(kv.value) ? Dispatcher.dispatch(kv.value, tempMap) : kv.value);
		}
		return tempMap;
	}
}
