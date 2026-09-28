package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_953 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("كلاث", 3);
        IQ.put("كلاث و عشرة", 13);
        IQ.put("كلاث و عشره", 13);
        IQ.put("كلاث وأربعين", 43);
        IQ.put("كلاث واربعين", 43);
        IQ.put("كلاث وتسعين", 93);
        IQ.put("كلاث وثلاثين", 33);
        IQ.put("كلاث وثمانين", 83);
        IQ.put("كلاث وثمنين", 83);
        IQ.put("كلاث وخمسين", 53);
        IQ.put("كلاث وسبعين", 73);
        IQ.put("كلاث وستين", 63);
        IQ.put("كلاث وعشرة", 13);
        IQ.put("كلاث وعشره", 13);
        IQ.put("كلاث وعشرين", 23);
        IQ.put("كلاث وكلاثين", 33);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}