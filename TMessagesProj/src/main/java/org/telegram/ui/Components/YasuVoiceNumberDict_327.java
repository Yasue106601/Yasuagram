package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_327 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("تسعه", 9);
        IQ.put("تسعه عشر", 19);
        IQ.put("تسعه و عشرة", 19);
        IQ.put("تسعه و عشره", 19);
        IQ.put("تسعه وأربعين", 49);
        IQ.put("تسعه واربعين", 49);
        IQ.put("تسعه وتسعين", 99);
        IQ.put("تسعه وثلاثين", 39);
        IQ.put("تسعه وثمانين", 89);
        IQ.put("تسعه وثمنين", 89);
        IQ.put("تسعه وخمسين", 59);
        IQ.put("تسعه وسبعين", 79);
        IQ.put("تسعه وستين", 69);
        IQ.put("تسعه وعشرة", 19);
        IQ.put("تسعه وعشره", 19);
        IQ.put("تسعه وعشرين", 29);
        IQ.put("تسعه وكلاثين", 39);
        FU.put("تسعه", 9);
        FU.put("تسعه عشر", 19);
        FU.put("تسعه و عشرة", 19);
        FU.put("تسعه و عشره", 19);
        FU.put("تسعه وأربعون", 49);
        FU.put("تسعه وأربعين", 49);
        FU.put("تسعه واربعون", 49);
        FU.put("تسعه واربعين", 49);
        FU.put("تسعه وتسعون", 99);
        FU.put("تسعه وتسعين", 99);
        FU.put("تسعه وثلاثون", 39);
        FU.put("تسعه وثلاثين", 39);
        FU.put("تسعه وثمانون", 89);
        FU.put("تسعه وثمانين", 89);
        FU.put("تسعه وخمسون", 59);
        FU.put("تسعه وخمسين", 59);
        FU.put("تسعه وسبعون", 79);
        FU.put("تسعه وسبعين", 79);
        FU.put("تسعه وستون", 69);
        FU.put("تسعه وستين", 69);
        FU.put("تسعه وعشرة", 19);
        FU.put("تسعه وعشره", 19);
        FU.put("تسعه وعشرون", 29);
        FU.put("تسعه وعشرين", 29);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}