package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_1164 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("واحده", 1);
        IQ.put("واحده و عشرة", 11);
        IQ.put("واحده و عشره", 11);
        IQ.put("واحده وأربعين", 41);
        IQ.put("واحده واربعين", 41);
        IQ.put("واحده وتسعين", 91);
        IQ.put("واحده وثلاثين", 31);
        IQ.put("واحده وثمانين", 81);
        IQ.put("واحده وثمنين", 81);
        IQ.put("واحده وخمسين", 51);
        IQ.put("واحده وسبعين", 71);
        IQ.put("واحده وستين", 61);
        IQ.put("واحده وعشرة", 11);
        IQ.put("واحده وعشره", 11);
        IQ.put("واحده وعشرين", 21);
        IQ.put("واحده وكلاثين", 31);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}