

import java.io.FileWriter;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;



public class RunSQL extends Thread {

	private String IP = "localhost";
	//
	private   String URL= "";
	private   String NAME="root";
	private   String PASSWORD="";
	private   String PORT="3306";
	private   String DB="ec";
	private   String TRANSACTION= "Transaction2";
	private Connection conn = null; 
	private int thread_no = 0;
	private int TPS_NUM = 100;
	//
	private   Transaction transaction;

	public RunSQL(int TPS_NUM,int thread_no,String IP,String IPLIST,String PASSWORD,String PORT,String DB,String TRANSACTION){
		this.TPS_NUM = TPS_NUM;
		this.IP = IP;
		this.PASSWORD = PASSWORD;
		this.PORT = PORT;
		this.DB = DB;
		this.TRANSACTION = TRANSACTION;
		if(IPLIST==null)
			URL = "jdbc:mysql://"+IP+":"+PORT+"/"+DB+"?";
		else
		{
			String[] iPStrings = IPLIST.split(",");
			int index = Randomdata.getRandomInteger(0, iPStrings.length-1);
			URL = "jdbc:mysql://"+iPStrings[index]+":"+PORT+"/"+DB+"?";
		}
		
		try {  
			Class.forName("com.mysql.jdbc.Driver");  
		} catch (ClassNotFoundException e) {  
			e.printStackTrace();  
		}  
		try {  
			conn = DriverManager.getConnection(URL, NAME, PASSWORD);  

			switch (TRANSACTION) {
			case "Transaction2":
				transaction = new Transaction2(conn);
				break;
			case "Transaction1":
				transaction = new Transaction1(conn);
				break;
			case "Transaction3":
				transaction = new Transaction3(conn);
				break;
			case "Transaction4":
				transaction = new Transaction4(conn);
				break;
			case "Transaction5":
				transaction = new Transaction5(conn);
				break;
			default:
				transaction = new Transaction2(conn);
				break;
			}


			//conn.setAutoCommit(false); 
		} catch (SQLException e) {  
			System.out.println("获取数据库连接失败！");  
			e.printStackTrace();  
		}  


	}

	public void run() {
		
		for (int i = 0; i < TPS_NUM; i++) {
			try {
				transaction.execute();
			} catch (SQLException e) {
				// TODO Auto-generated catch block 
				//e.printStackTrace();
			}
		}
		if(conn!=null)  
		{  
			try {  
				conn.close();  
			} catch (SQLException e) {  
				e.printStackTrace();  
				conn=null;  
			}  
		}  
	}

}
