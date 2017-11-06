import java.util.Formatter;
import java.util.PrimitiveIterator.OfDouble;

import gnu.getopt.Getopt;
import jdk.internal.dynalink.beans.StaticClass;

public class main {
	public	static Formatter formatter = new Formatter(System.out);
	public static boolean STARTTRANSACTION = true;
	public static void main(String[] args) {
		String IP = "localhost";
		String PASSWORD = "";
		String DB = "ec";
		String PORT = "3306";
		String TRANSACTION = "Transaction2";
		String IPLIST = null;
		int THREADNUM = 200;
		int OPPERTHREAD = 10;
		if(args.length!=0)
		{
			Getopt testOpt  = new Getopt(args[0], args, "H:h:p:c:P:D:O:T:t:");  
			int res;  
			while( (res = testOpt.getopt()) != -1 ) {  
				switch(res) {  
				case 'h':  
					IP =  testOpt.getOptarg();  
					break;  
				case 'H':  
					IPLIST =  testOpt.getOptarg();  
					break;
				case 'p':  
					PASSWORD = testOpt.getOptarg();  
					break;  
				case 'c':  
					THREADNUM = Integer.parseInt(testOpt.getOptarg());  
					break;
				case 'P':  
					PORT = testOpt.getOptarg();  
					break; 
				case 't':  
					if( testOpt.getOptarg().compareTo("yes")==0|| testOpt.getOptarg().compareTo("no")==0)
					{
						if(testOpt.getOptarg()=="yes")
							STARTTRANSACTION = true;
						else
							STARTTRANSACTION = false;
					}
					else {
						return;
					}
					
					break; 
				case 'D':  
					DB = testOpt.getOptarg();  
					break; 
				case 'O':  
					OPPERTHREAD = Integer.parseInt(testOpt.getOptarg());  
					break;	
				case 'T':  
					TRANSACTION = testOpt.getOptarg();  
					break; 
				default:  
					System.out.println("input ip and password!");  
					return;
				}  
			}  
		}
		else
		{
			printHelp();
			return;
		}
		// TODO Auto-generated method stub
		int threadNum = THREADNUM;
		int TPPTHREAD = OPPERTHREAD;
		System.out.println("==========================================================");
		formatter.format("%-35s %-25s\n", "Work thread num:", threadNum+" --default 200");
		formatter.format("%-35s %d\n", "Select per thread is:", TPPTHREAD);
		formatter.format("%-35s %d\n", "Total  ops:", TPPTHREAD*threadNum);
		System.out.println("Running......");
		System.out.println("==========================================================");
		long runSqlTotalTime = 0;
		RunSQL[] thread_sql=new RunSQL[threadNum];
		long startTime =System.currentTimeMillis(); 
		System.out.println("Start......");

		for(int i=0;i<threadNum;i++)
		{
			RunSQL rs = new RunSQL(TPPTHREAD,i,IP,IPLIST,PASSWORD,PORT,DB,TRANSACTION);
			thread_sql[i] = rs;
			rs.start();
		}
		try
		{
			for(int i=0;i<threadNum;i++)
			{
				thread_sql[i].join();
			}

		} catch
		(InterruptedException e) {
			e.printStackTrace();
		}
		long endTime =System.currentTimeMillis();
		runSqlTotalTime = endTime - startTime;
		System.out.println("Total run time: "+runSqlTotalTime+"ms");
		System.out.println("TPS: " + ((float)(threadNum*TPPTHREAD)/(runSqlTotalTime))*1000+"/s" );
	}
	public static void printHelp() {
		System.out.println("INFO :");
		System.out.println("           Created By wanglei244@huawei.com");
		System.out.println("USAGE :");
		System.out.println("Command line options :");
		System.out.println("");
		formatter.format("   %-15s %-55s\n", "-h", "Connect to host.");
		formatter.format("   %-15s %-55s\n", "-H", "Connect to host list.");
		formatter.format("   %-15s %-55s\n", "-p", "Database port.");
		formatter.format("   %-15s %-55s\n", "-c", "Max connections.");
		formatter.format("   %-15s %-55s\n", "-p", "Password.");
		formatter.format("   %-15s %-55s\n", "-D", "Database.");
		formatter.format("   %-15s %-55s\n", "-O", "Operation per thread.");
		formatter.format("   %-15s %-55s\n", "-T", "Transaction test.");
		System.out.println("SAMPLE :");
		System.out.println("java -jar tps.jar -h192.168.1.234 -p3306 -Dec -c500 -O100 -T Transaction3");
		
	}

}
