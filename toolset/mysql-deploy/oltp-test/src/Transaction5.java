import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;


public class Transaction5 implements Transaction{
	 private static final String sql1 = "select nSKUID from r_ec_cartsku where nUserID=?;";
	 private static final String sql3 = "select count(*) from r_ec_cartsku where nUserID=?;";
	 private static final String sql2 = "update r_ec_cartsku set nQuantity=123 where nUserID=? and nSKUID=?;";
     private PreparedStatement pstmt1 = null;
     private PreparedStatement pstmt2 = null;
     private PreparedStatement pstmt3 = null;
     private Connection conn = null; 
     private Statement  stmt = null;
     private ResultSet retsult = null;
	 public Transaction5(Connection conn) throws SQLException
	 {
		 this.conn = conn;
		 pstmt1 =  conn.prepareStatement(sql1);
		 pstmt2 =  conn.prepareStatement(sql2);
		 pstmt3 =  conn.prepareStatement(sql3);
		 stmt = conn.createStatement(); 
	 }
	 public void execute() throws SQLException
	 {
		 int nUserID = Randomdata.getRandomInteger(1, 1000000);
		
		 pstmt1.setInt(1, nUserID);
		 pstmt2.setInt(1, nUserID);
		 pstmt3.setInt(1, nUserID);
		 
		 retsult = pstmt3.executeQuery();
		 int row_count =0;
		 retsult.next();
		 row_count = retsult.getInt(0);
		 retsult = null;
		 retsult = pstmt1.executeQuery();
		 int[] skuidList = new int[row_count];
		 int i=0;
		 while (retsult.next()) {
			 skuidList[i++] = retsult.getInt("nSKUID");
			 System.out.println("nSKUID: "+ retsult.getInt("nSKUID"));
		 }
		 
		 int nSKUID =skuidList[Randomdata.getRandomInteger(0,skuidList.length-1)];
		 System.out.println("nSKUID");
		 pstmt2.setInt(2, nSKUID);
		 
		 
		 if(main.STARTTRANSACTION)
			 stmt.executeQuery("start transaction");
		 pstmt2.executeUpdate();
		 if(main.STARTTRANSACTION)
			 stmt.executeQuery("commit");
	 }
	 public void printSQL()
	 {
		 System.out.println("Transaction SQL");
		 System.out.println(sql1);
		 System.out.println("----------------------------------------------------------------------");
	 }
}
