package src.main.java.cn.net.communion.main;

import java.io.IOException;
import java.util.Formatter;
import java.util.List;

import src.main.java.cn.net.communion.core.Generator;
import src.main.java.cn.net.communion.entity.JobInfo;
import src.main.java.cn.net.communion.helper.ArgParser;
import src.main.java.cn.net.communion.xml.Root;



public class App {
	public static int KEY =1;
	public static void main(String[] args) {
		if(args.length!=0)
		{
			ArgParser.argParser(args);
		}
		else
		{
			ArgParser.printHelp();
			return;
		}
		Root root = Root.getInstance().loadXml(ArgParser.JOBFILE);
		List<JobInfo> ll=root.getJobList();
		if (root != null) {

			try {
				new Generator().start(root.getJobList());
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

	}


}
