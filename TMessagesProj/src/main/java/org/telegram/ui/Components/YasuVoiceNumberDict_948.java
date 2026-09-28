package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_948 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("صطعش", 16);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}