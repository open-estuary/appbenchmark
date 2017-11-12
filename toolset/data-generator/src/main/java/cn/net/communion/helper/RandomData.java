package src.main.java.cn.net.communion.helper;

import java.util.Random;
import java.util.regex.Pattern;

public class RandomData {
	static private Random rand = new Random();

	static public String getRuleData(String content, int len) {
		StringBuffer result = new StringBuffer();
		if(!isInteger(content))
		{
			int strLen = content.length();
			for (int i = 0; i < len; i++) {
				result.append(content.charAt(rand.nextInt(strLen)));
			}
			return result.toString();
		}
		else
		{
			return getRandomInteger(content); 
		}
		
	}

	static public String getPoolData(String[] arr) {
		return arr[rand.nextInt(arr.length)];
	}

	static public boolean isInteger(String str)
	{
		Pattern pattern = Pattern.compile("^[-\\+]?[\\d]*$");
		return pattern.matcher(str).matches();
	}
	static public String getRandomInteger(String content)
	{
		int max = Integer.parseInt(content);
		int min =1;
		Random rand = new Random();
		int res = rand.nextInt(max)%(max-min+1)+min;
		String res_str = Integer.toString(res);
		return res_str;
	}
}
