import java.util.Random;

public class Randomdata {
	 public static int getRandomInteger(int start,int end)
	    {
	    	Random random = new Random();
			return random.nextInt(end)%(end-start+1) + start;
	    }
}
