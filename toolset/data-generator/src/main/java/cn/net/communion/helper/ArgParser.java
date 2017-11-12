package src.main.java.cn.net.communion.helper;

import java.util.Formatter;

import gnu.getopt.Getopt;
import gnu.getopt.LongOpt;

public class ArgParser {
	public static String IP = "localhost";
	public static String PASSWORD = "";
	public static String PORT = "3306";
	public static int THREADNUM = 200;
	public static String DB = "ec";
	public static String JOBFILE = "jobs.xml";
	public static String CONFIGFILE = "config.properties";
	public static String INSERTORREPLACE = "replace";
	public static String DATABASEOP = "open";
	public static String FILETYPE = "";
	public static String SEPARATOR = " ";
	public	static Formatter formatter = new Formatter(System.out);
	public static int argParser(String[] args)
	{
		int res = ErrorCode.SUCCESS;
		LongOpt[] longopts = new LongOpt[11]; 
		longopts[0] = new LongOpt("ip", LongOpt.NO_ARGUMENT, null,'h');  
		longopts[1] = new LongOpt("password", LongOpt.REQUIRED_ARGUMENT,null, 'p');  
		longopts[2] = new LongOpt("port", LongOpt.REQUIRED_ARGUMENT,null, 'P'); 
		longopts[3] = new LongOpt("thread_num", LongOpt.REQUIRED_ARGUMENT,null, 'c'); 
		longopts[4] = new LongOpt("port", LongOpt.REQUIRED_ARGUMENT,null, 'P'); 
		longopts[5] = new LongOpt("jobfile", LongOpt.REQUIRED_ARGUMENT,null, 'j');
		longopts[6] = new LongOpt("database", LongOpt.REQUIRED_ARGUMENT,null, 'D');
		longopts[7] = new LongOpt("configfile", LongOpt.REQUIRED_ARGUMENT,null, 'C');
		longopts[8] = new LongOpt("insert_or_repalce", LongOpt.REQUIRED_ARGUMENT,null, 'C');
		longopts[9] = new LongOpt("if_database_operation", LongOpt.REQUIRED_ARGUMENT,null, 'O');
		longopts[10] = new LongOpt("out_put_file_type", LongOpt.REQUIRED_ARGUMENT,null, 't');
		Getopt getOpt  = new Getopt(args[0], args, "i:h:p:c:j:P:D:C:O:t:S:");  
		int ch;  
		while( (ch = getOpt.getopt()) != -1 ) {  
			switch(ch) {  
			case 'i':  
				INSERTORREPLACE =  getOpt.getOptarg();  
				if(INSERTORREPLACE!="insert" ||INSERTORREPLACE!="replace")
					res = ErrorCode.ERR_ARG_TYPE;
				break;
			case 'h':  
				IP =  getOpt.getOptarg();  
				break;  
			case 'p':  
				PASSWORD = getOpt.getOptarg();  
				break; 
			case 'j':  
				JOBFILE = getOpt.getOptarg();  
				break; 
			case 'c':  
				THREADNUM = Integer.parseInt(getOpt.getOptarg());  
				break; 
			case 't':  
				FILETYPE =  getOpt.getOptarg();  
				break; 
			case 'C':  
				CONFIGFILE = getOpt.getOptarg(); 
				break; 
			case 'S':  
				SEPARATOR = getOpt.getOptarg(); 
				break; 
			case 'P':  
				PORT = getOpt.getOptarg(); 
				break; 
			case 'D':  
				DB = getOpt.getOptarg();  
				break; 
			case 'O':  
				DATABASEOP = getOpt.getOptarg();  
				if(DATABASEOP!="open" ||DATABASEOP!="close")
					res = ErrorCode.ERR_ARG_TYPE;
				break; 
			default:  
				System.out.println("input ip and password!");  
				res = ErrorCode.ERR_ARG_TYPE;
				break;
			}  
		} 
		return res;
	}
	public static void printHelp() {
		System.out.println("INFO :");
		System.out.println("           Created By wanglei244@huawei.com");
		System.out.println("USAGE :");
		System.out.println("Command line options :");
		System.out.println("");
		formatter.format("   %-15s %-55s\n", "-h/--ip", "Database host, default localhost.");
		formatter.format("   %-15s %-55s\n", "-p/--port", "Database port, default 3306.");
		formatter.format("   %-15s %-55s\n", "-c/--thread_num", "Max connections, default 200");
		formatter.format("   %-15s %-55s\n", "-p/--password", "Password, default is empty string.");
		formatter.format("   %-15s %-55s\n", "-D/--database", "Database, default ec.");
		formatter.format("   %-15s %-55s\n", "-j/--jobfile", "Job files, default jobs.xml.");
		formatter.format("   %-15s %-55s\n", "-t/--out_put_file_type", "Out put file type, default none, support type 'csv'.");
		formatter.format("   %-15s %-55s\n", "-S", "Separators for each record,default is empty string.");
		formatter.format("   %-15s %-55s\n", "-C/--configfile", "Configure files, default config.properties.");
		formatter.format("   %-15s %-55s\n", "-i/--insert_or_repalce", "Insert or Replace, two choice 'insert' or 'replave',default replace.");
		formatter.format("   %-15s %-55s\n", "-O/--if_database_operation", "Open or close database operation,two choice 'open' or 'close', default open.");
		System.out.println("SAMPLE :");
		System.out.println("java -jar data-generator -h192.168.1.234 -p3306 -Dec -c500 -j jobs.xml -C config.properties -i insert");
		System.out.println("java -jar data-generator -h192.168.1.234 -O close");
		System.out.println("java -jar data-generator.jar -O close -t csv");
		System.out.println("java -jar data-generator.jar -O close -t csv -S space");
		System.out.println("java -jar data-generator.jar -O close -t csv -S comma");
	}
}
