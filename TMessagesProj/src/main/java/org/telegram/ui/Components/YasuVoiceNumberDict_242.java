package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_242 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("تسعة", 9);
        IQ.put("تسعة عشر", 19);
        IQ.put("تسعة و عشرة", 19);
        IQ.put("تسعة و عشره", 19);
        IQ.put("تسعة وأربعين", 49);
        IQ.put("تسعة واربعين", 49);
        IQ.put("تسعة وتسعين", 99);
        IQ.put("تسعة وثلاثين", 39);
        IQ.put("تسعة وثمانين", 89);
        IQ.put("تسعة وثمنين", 89);
        IQ.put("تسعة وخمسين", 59);
        IQ.put("تسعة وسبعين", 79);
        IQ.put("تسعة وستين", 69);
        IQ.put("تسعة وعشرة", 19);
        IQ.put("تسعة وعشره", 19);
        IQ.put("تسعة وعشرين", 29);
        IQ.put("تسعة وكلاثين", 39);
        FU.put("تسعة", 9);
        FU.put("تسعة عشر", 19);
        FU.put("تسعة و عشرة", 19);
        FU.put("تسعة و عشره", 19);
        FU.put("تسعة وأربعون", 49);
        FU.put("تسعة وأربعين", 49);
        FU.put("تسعة واربعون", 49);
        FU.put("تسعة واربعين", 49);
        FU.put("تسعة وتسعون", 99);
        FU.put("تسعة وتسعين", 99);
        FU.put("تسعة وثلاثون", 39);
        FU.put("تسعة وثلاثين", 39);
        FU.put("تسعة وثمانون", 89);
        FU.put("تسعة وثمانين", 89);
        FU.put("تسعة وخمسون", 59);
        FU.put("تسعة وخمسين", 59);
        FU.put("تسعة وسبعون", 79);
        FU.put("تسعة وسبعين", 79);
        FU.put("تسعة وستون", 69);
        FU.put("تسعة وستين", 69);
        FU.put("تسعة وعشرة", 19);
        FU.put("تسعة وعشره", 19);
        FU.put("تسعة وعشرون", 29);
        FU.put("تسعة وعشرين", 29);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}