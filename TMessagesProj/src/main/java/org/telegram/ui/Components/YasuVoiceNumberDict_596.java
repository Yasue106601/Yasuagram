package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_596 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("ثنين", 2);
        IQ.put("ثنين و عشرة", 12);
        IQ.put("ثنين و عشره", 12);
        IQ.put("ثنين وأربعين", 42);
        IQ.put("ثنين واربعين", 42);
        IQ.put("ثنين وتسعين", 92);
        IQ.put("ثنين وثلاثين", 32);
        IQ.put("ثنين وثمانين", 82);
        IQ.put("ثنين وثمنين", 82);
        IQ.put("ثنين وخمسين", 52);
        IQ.put("ثنين وسبعين", 72);
        IQ.put("ثنين وستين", 62);
        IQ.put("ثنين وعشرة", 12);
        IQ.put("ثنين وعشره", 12);
        IQ.put("ثنين وعشرين", 22);
        IQ.put("ثنين وكلاثين", 32);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}