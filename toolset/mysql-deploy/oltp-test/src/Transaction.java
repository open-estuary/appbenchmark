import java.sql.SQLException;

public interface Transaction {
	 public void execute()throws SQLException ;
	 public void printSQL();
}
