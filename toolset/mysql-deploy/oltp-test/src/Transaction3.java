
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;

public class Transaction3  implements Transaction{

	 private static final String sql1 = "select * from r_ec_cartsku ct join r_ec_sku sku on ct.nSKUID = sku.nSKUID where ct.nUserID=?";
     private PreparedStatement pstmt1 = null;

     private Connection conn = null; 
     private Statement  stmt = null;
	 public Transaction3(Connection conn) throws SQLException
	 {
		 this.conn = conn;
		 pstmt1 =  conn.prepareStatement(sql1);

		 stmt = conn.createStatement(); 
	 }
	 public void execute() throws SQLException
	 {
		 int nUserID = Randomdata.getRandomInteger(1, 1000000);
		 pstmt1.setInt(1, nUserID);

		 if(main.STARTTRANSACTION)
			 stmt.executeQuery("start transaction");
		 pstmt1.executeQuery();
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
