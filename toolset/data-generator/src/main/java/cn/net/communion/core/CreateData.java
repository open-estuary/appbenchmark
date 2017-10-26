package src.main.java.cn.net.communion.core;

import java.io.FileWriter;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import src.main.java.cn.net.communion.entity.JobInfo;
import src.main.java.cn.net.communion.entity.KeyValue;
import src.main.java.cn.net.communion.helper.FileHelper;

public class CreateData extends Thread {
	private FileWriter writer;
	private Map<String, String> tempMap = new HashMap<String, String>();
	private List<KeyValue> detail = new ArrayList<KeyValue>(); 
	private List<String> sqlList;
	private JobInfo job ;
	private int start =0;
	private int end =0;
	private Generator generator =new Generator();
	public CreateData( FileWriter write,List<KeyValue> detail,List<String> sqlList,JobInfo job,int start,int end)
	{
		this.writer=write;
		this.detail=detail;
		this.sqlList=sqlList;
		this.job=job;
		this.start=start;
		this.end=end;
	}
	public void run() {
		 for (int i = start; i < end; i++) {
			 generator.fillTempMap(detail, tempMap);
        	 FileHelper.save(writer,job.getTable(), tempMap );
        	 String insertSQL = FileHelper.getInsertSQL(job.getTable(), tempMap).toString();
        	 sqlList.add(insertSQL);
             tempMap.clear();  
         }
	 }
}
