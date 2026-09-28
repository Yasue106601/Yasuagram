package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_209 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("الف", 1000);
        FU.put("الف", 1000);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}