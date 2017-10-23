
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;

public class Transaction1  implements Transaction{
	 private static final String sql1 = "select * from r_ec_userinfo where nUserID=?;";
	 private static final String sql2 = "select * from r_ec_cartsku ct join r_ec_sku sku on ct.nSKUID = sku.nSKUID where ct.nUserID=?";
	 private static final String sql3 = "update r_ec_cartsku set nQuantity=123 where nUserID=?;";
     private PreparedStatement pstmt1 = null;
     private PreparedStatement pstmt2 = null;
     private PreparedStatement pstmt3 = null;
     private Connection conn = null; 
     private Statement  stmt = null;
	 public Transaction1(Connection conn) throws SQLException
	 {
		 this.conn = conn;
		 pstmt1 =  conn.prepareStatement(sql1);
		 pstmt2 =  conn.prepareStatement(sql2);
		 pstmt3 =  conn.prepareStatement(sql3);
		 stmt   =  conn.createStatement(); 
	 }
	 public void execute() throws SQLException
	 {
		 int nUserID = Randomdata.getRandomInteger(1, 1000000);
		 pstmt1.setInt(1, nUserID);
		 pstmt2.setInt(1, nUserID);
		 pstmt3.setInt(1, nUserID);
		 if(main.STARTTRANSACTION)
			 stmt.executeQuery("start transaction");
		 pstmt1.executeQuery();
		 pstmt2.executeQuery();
		 pstmt3.executeUpdate();
		 if(main.STARTTRANSACTION)
			 stmt.executeQuery("commit");
	 }
	 public void printSQL()
	 {
		 System.out.println("Transaction SQL");
		 System.out.println(sql1);
		 System.out.println(sql2);
		 System.out.println(sql3);
		 System.out.println("----------------------------------------------------------------------");
	 }
}
