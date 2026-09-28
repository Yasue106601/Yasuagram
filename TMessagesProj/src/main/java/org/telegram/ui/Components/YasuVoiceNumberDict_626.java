package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_626 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("خمسة", 5);
        IQ.put("خمسة عشر", 15);
        IQ.put("خمسة و عشرة", 15);
        IQ.put("خمسة و عشره", 15);
        IQ.put("خمسة وأربعين", 45);
        IQ.put("خمسة واربعين", 45);
        IQ.put("خمسة وتسعين", 95);
        IQ.put("خمسة وثلاثين", 35);
        IQ.put("خمسة وثمانين", 85);
        IQ.put("خمسة وثمنين", 85);
        IQ.put("خمسة وخمسين", 55);
        IQ.put("خمسة وسبعين", 75);
        IQ.put("خمسة وستين", 65);
        IQ.put("خمسة وعشرة", 15);
        IQ.put("خمسة وعشره", 15);
        IQ.put("خمسة وعشرين", 25);
        IQ.put("خمسة وكلاثين", 35);
        FU.put("خمسة", 5);
        FU.put("خمسة عشر", 15);
        FU.put("خمسة و عشرة", 15);
        FU.put("خمسة و عشره", 15);
        FU.put("خمسة وأربعون", 45);
        FU.put("خمسة وأربعين", 45);
        FU.put("خمسة واربعون", 45);
        FU.put("خمسة واربعين", 45);
        FU.put("خمسة وتسعون", 95);
        FU.put("خمسة وتسعين", 95);
        FU.put("خمسة وثلاثون", 35);
        FU.put("خمسة وثلاثين", 35);
        FU.put("خمسة وثمانون", 85);
        FU.put("خمسة وثمانين", 85);
        FU.put("خمسة وخمسون", 55);
        FU.put("خمسة وخمسين", 55);
        FU.put("خمسة وسبعون", 75);
        FU.put("خمسة وسبعين", 75);
        FU.put("خمسة وستون", 65);
        FU.put("خمسة وستين", 65);
        FU.put("خمسة وعشرة", 15);
        FU.put("خمسة وعشره", 15);
        FU.put("خمسة وعشرون", 25);
        FU.put("خمسة وعشرين", 25);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}