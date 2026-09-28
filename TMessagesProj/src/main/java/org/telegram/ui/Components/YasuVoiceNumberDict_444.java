package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_444 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("ثلاثه", 3);
        IQ.put("ثلاثه عشر", 13);
        IQ.put("ثلاثه و عشرة", 13);
        IQ.put("ثلاثه و عشره", 13);
        IQ.put("ثلاثه وأربعين", 43);
        IQ.put("ثلاثه واربعين", 43);
        IQ.put("ثلاثه وتسعين", 93);
        IQ.put("ثلاثه وثلاثين", 33);
        IQ.put("ثلاثه وثمانين", 83);
        IQ.put("ثلاثه وثمنين", 83);
        IQ.put("ثلاثه وخمسين", 53);
        IQ.put("ثلاثه وسبعين", 73);
        IQ.put("ثلاثه وستين", 63);
        IQ.put("ثلاثه وعشرة", 13);
        IQ.put("ثلاثه وعشره", 13);
        IQ.put("ثلاثه وعشرين", 23);
        IQ.put("ثلاثه وكلاثين", 33);
        FU.put("ثلاثه", 3);
        FU.put("ثلاثه عشر", 13);
        FU.put("ثلاثه و عشرة", 13);
        FU.put("ثلاثه و عشره", 13);
        FU.put("ثلاثه وأربعون", 43);
        FU.put("ثلاثه وأربعين", 43);
        FU.put("ثلاثه واربعون", 43);
        FU.put("ثلاثه واربعين", 43);
        FU.put("ثلاثه وتسعون", 93);
        FU.put("ثلاثه وتسعين", 93);
        FU.put("ثلاثه وثلاثون", 33);
        FU.put("ثلاثه وثلاثين", 33);
        FU.put("ثلاثه وثمانون", 83);
        FU.put("ثلاثه وثمانين", 83);
        FU.put("ثلاثه وخمسون", 53);
        FU.put("ثلاثه وخمسين", 53);
        FU.put("ثلاثه وسبعون", 73);
        FU.put("ثلاثه وسبعين", 73);
        FU.put("ثلاثه وستون", 63);
        FU.put("ثلاثه وستين", 63);
        FU.put("ثلاثه وعشرة", 13);
        FU.put("ثلاثه وعشره", 13);
        FU.put("ثلاثه وعشرون", 23);
        FU.put("ثلاثه وعشرين", 23);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}