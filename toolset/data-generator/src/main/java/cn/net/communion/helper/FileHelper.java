package src.main.java.cn.net.communion.helper;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Collection;
import java.util.Map;
import java.util.Set;

public class FileHelper {
    static public String read(String path) {
        StringBuffer result = new StringBuffer();
        try {
            FileInputStream in = new FileInputStream(path);
            InputStreamReader r = new InputStreamReader(in, "UTF-8");
            BufferedReader br = new BufferedReader(r);
            String line = null;
            while ((line = br.readLine()) != null) {
                result.append(line).append(",");
            }
            br.close();
            r.close();
            in.close();
            return result.toString().replaceAll(",$", "");
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    static public synchronized  void save(FileWriter writer, String table, Map<String, String> map) {
        if(writer == null && (table == null || "".equals(table.trim()))){
            System.out.println(map);
            return;
        }
        StringBuffer line = new StringBuffer(ArgParser.INSERTORREPLACE+" into ").append(table).append(" (").append(collectionToLine(map.keySet())).append(")").append(" values(")
                        .append(collectionToLineWithQuotes(map.values())).append(");");
        if (writer != null) {
            save(writer, line);
        } else {
            System.out.println(line);
        }
    }
    static public synchronized  void saveAsCSV(FileWriter writer, String table, Map<String, String> map) {
        if(writer == null && (table == null || "".equals(table.trim()))){
            System.out.println(map);
            return;
        }
        StringBuffer line =  new StringBuffer().append(collectionToLineWithQuotes(map.values()," "));
        if (writer != null) {
            save(writer, line);
        } else {
            System.out.println(line);
        }
    }
    static public synchronized  StringBuffer getInsertSQL(String table, Map<String, String> map) {
        if(table == null || "".equals(table.trim())){
            System.out.println(map);
            return null;
        }
        StringBuffer line = new StringBuffer(ArgParser.INSERTORREPLACE+" into ").append(table).append(" (").append(collectionToLine(map.keySet())).append(")").append(" values(")
                        .append(collectionToLineWithQuotes(map.values())).append(");");
        return line;

    }
    
    static public void save(FileWriter writer, Collection<String> col) {
        save(writer, collectionToLine(col));
    }

    private static void save(FileWriter writer, StringBuffer line) {
        try {
            writer.write(line.append("\n").toString());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static StringBuffer collectionToLine(Collection<String> collection) {
        StringBuffer line = new StringBuffer();
        for (String value : collection) {
            line.append(value).append(",");
        }
        return line.replace(line.length() - 1, line.length(), "");
    }

    private static StringBuffer collectionToLineWithQuotes(Collection<String> collection) {
        StringBuffer line = new StringBuffer();
        for (String value : collection) {
            line.append("'").append(value).append("',");
        }
        return line.replace(line.length() - 1, line.length(), "");
    }
    private static StringBuffer collectionToLineWithQuotes(Collection<String> collection,String split) {
        StringBuffer line = new StringBuffer();
        for (String value : collection) {
            line.append("").append(value).append(split);
        }
        return line.replace(line.length() - 1, line.length(), "");
    }
}
