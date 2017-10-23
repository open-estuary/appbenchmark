package src.main.java.cn.net.communion.core;


import java.text.SimpleDateFormat;
import java.time.temporal.ChronoUnit;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Random;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import src.main.java.cn.net.communion.helper.FileHelper;
import src.main.java.cn.net.communion.helper.PropsReader;
import src.main.java.cn.net.communion.helper.RandomData;
import src.main.java.cn.net.communion.main.App;



public class Parser {
    private String value;
    private Map<String, String> map;
    private Pattern varPattern;
    private Pattern rulePattern;
    private Pattern poolPattern;
    private Pattern primaryKeyPattern;
    private Pattern datePattern;
    private PropsReader props;
    private Map<String, String> poolFileMap = new HashMap<String, String>();
    static private Parser instance = null;
    private Random rand = new Random();

    private Parser() {
        varPattern = Pattern.compile("\\$var\\{(\\w+)\\}");
        rulePattern = Pattern.compile("\\$rule\\{([0-9a-zA-Z,]+)\\}");
        poolPattern = Pattern.compile("\\$pool\\{([0-9a-zA-Z.]+)\\}");
        primaryKeyPattern = Pattern.compile("\\$key");
        datePattern = Pattern.compile("\\$datetime");
        props = PropsReader.getInstance();

    }

    static Parser getInstance(String value, Map<String, String> map) {
        if (instance == null) {
            instance = new Parser();
        }
        instance.setValue(value);
        instance.setMap(map);
        return instance;
    }

    static public boolean checkGrammar(String value) {
        return value.contains("$var") || value.contains("$rule") || value.contains("$pool")
                || value.contains("$key")|| value.contains("$datetime");
    }

    public String execute() {
        parseVar().parseRule().parsePool().parseKey().parseDate();
        return value;
    }

    private Parser parseVar() {
        Matcher m = varPattern.matcher(value);
        if (m.find()) {
            String name = m.group(1);
            String propValue = props.getProperty("var." + name);
            value = value.replace(m.group(0), propValue != null ? propValue : this.map.get(name));
        }
        return this;
    }

    private Parser parseRule() {
        Matcher m = rulePattern.matcher(value);
        if (m.find()) {
            value = value.replace(m.group(0), getRuleData(m.group(1).split(",")));
        }
        return this;
    }


    private String getRuleData(String[] arr) {
        String content = props.getProperty("rule." + arr[0]);
        if (content != null) {
            return RandomData.getRuleData(content, arr.length < 2 ? 6 : Integer.parseInt(arr[1]));
        }
        return null;
    }

    private Parser parsePool() {
        Matcher m = poolPattern.matcher(value);
        if (m.find()) {
            value = value.replace(m.group(0), getPoolData(m.group(1)));
        }
        return this;
    }

    private String getPoolData(String name) {
        String content = props.getProperty("pool." + name);

        if (content != null) {
            return RandomData.getPoolData(content.split(","));
        } else {
            String poolContent = poolFileMap.get(name);
            if (poolContent == null) {
                poolContent = FileHelper.read("pool/" + name);
                if (poolContent == null) {
                    return null;
                }
                poolFileMap.put(name, poolContent);
            }
            return RandomData.getPoolData(poolContent.split(","));
        }
    }
    
    private Parser parseKey() {
        Matcher m = primaryKeyPattern.matcher(value);
        if (m.find()) {
            value = value.replace(m.group(0), getKeyData());
        }
        return this;
    }
    
    private synchronized String getKeyData() {
    	String Key = Integer.toString(App.KEY);
    	App.KEY++;
    	return Key;
    }
    
    private Parser parseDate() {
        Matcher m = datePattern.matcher(value);
        if (m.find()) {
            value = value.replace(m.group(0), getDateData());
        }
        return this;
    }
    
    private synchronized String getDateData() {
    	SimpleDateFormat date_now = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    	return date_now.format(new Date()).toString();
    }
    public String getValue() {
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }

    public Map<String, String> getMap() {
        return map;
    }

    public void setMap(Map<String, String> map) {
        this.map = map;
    }
}
