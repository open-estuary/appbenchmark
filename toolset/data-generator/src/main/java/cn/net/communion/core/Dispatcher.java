package src.main.java.cn.net.communion.core;

import java.util.Map;

public class Dispatcher {
    static public synchronized  String dispatch(String value, Map<String, String> map) {
        return Parser.getInstance(value, map).execute();
    }

    static public synchronized  boolean checkGrammer(String value) {
        return Parser.checkGrammar(value);
    }
}
