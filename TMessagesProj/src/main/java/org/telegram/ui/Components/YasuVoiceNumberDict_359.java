package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_359 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("ثلاثة", 3);
        IQ.put("ثلاثة عشر", 13);
        IQ.put("ثلاثة و عشرة", 13);
        IQ.put("ثلاثة و عشره", 13);
        IQ.put("ثلاثة وأربعين", 43);
        IQ.put("ثلاثة واربعين", 43);
        IQ.put("ثلاثة وتسعين", 93);
        IQ.put("ثلاثة وثلاثين", 33);
        IQ.put("ثلاثة وثمانين", 83);
        IQ.put("ثلاثة وثمنين", 83);
        IQ.put("ثلاثة وخمسين", 53);
        IQ.put("ثلاثة وسبعين", 73);
        IQ.put("ثلاثة وستين", 63);
        IQ.put("ثلاثة وعشرة", 13);
        IQ.put("ثلاثة وعشره", 13);
        IQ.put("ثلاثة وعشرين", 23);
        IQ.put("ثلاثة وكلاثين", 33);
        FU.put("ثلاثة", 3);
        FU.put("ثلاثة عشر", 13);
        FU.put("ثلاثة و عشرة", 13);
        FU.put("ثلاثة و عشره", 13);
        FU.put("ثلاثة وأربعون", 43);
        FU.put("ثلاثة وأربعين", 43);
        FU.put("ثلاثة واربعون", 43);
        FU.put("ثلاثة واربعين", 43);
        FU.put("ثلاثة وتسعون", 93);
        FU.put("ثلاثة وتسعين", 93);
        FU.put("ثلاثة وثلاثون", 33);
        FU.put("ثلاثة وثلاثين", 33);
        FU.put("ثلاثة وثمانون", 83);
        FU.put("ثلاثة وثمانين", 83);
        FU.put("ثلاثة وخمسون", 53);
        FU.put("ثلاثة وخمسين", 53);
        FU.put("ثلاثة وسبعون", 73);
        FU.put("ثلاثة وسبعين", 73);
        FU.put("ثلاثة وستون", 63);
        FU.put("ثلاثة وستين", 63);
        FU.put("ثلاثة وعشرة", 13);
        FU.put("ثلاثة وعشره", 13);
        FU.put("ثلاثة وعشرون", 23);
        FU.put("ثلاثة وعشرين", 23);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}