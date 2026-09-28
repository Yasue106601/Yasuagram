package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_1165 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("وحده", 1);
        IQ.put("وحده و عشرة", 11);
        IQ.put("وحده و عشره", 11);
        IQ.put("وحده وأربعين", 41);
        IQ.put("وحده واربعين", 41);
        IQ.put("وحده وتسعين", 91);
        IQ.put("وحده وثلاثين", 31);
        IQ.put("وحده وثمانين", 81);
        IQ.put("وحده وثمنين", 81);
        IQ.put("وحده وخمسين", 51);
        IQ.put("وحده وسبعين", 71);
        IQ.put("وحده وستين", 61);
        IQ.put("وحده وعشرة", 11);
        IQ.put("وحده وعشره", 11);
        IQ.put("وحده وعشرين", 21);
        IQ.put("وحده وكلاثين", 31);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}