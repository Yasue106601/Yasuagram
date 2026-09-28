package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_711 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("خمسه", 5);
        IQ.put("خمسه عشر", 15);
        IQ.put("خمسه و عشرة", 15);
        IQ.put("خمسه و عشره", 15);
        IQ.put("خمسه وأربعين", 45);
        IQ.put("خمسه واربعين", 45);
        IQ.put("خمسه وتسعين", 95);
        IQ.put("خمسه وثلاثين", 35);
        IQ.put("خمسه وثمانين", 85);
        IQ.put("خمسه وثمنين", 85);
        IQ.put("خمسه وخمسين", 55);
        IQ.put("خمسه وسبعين", 75);
        IQ.put("خمسه وستين", 65);
        IQ.put("خمسه وعشرة", 15);
        IQ.put("خمسه وعشره", 15);
        IQ.put("خمسه وعشرين", 25);
        IQ.put("خمسه وكلاثين", 35);
        FU.put("خمسه", 5);
        FU.put("خمسه عشر", 15);
        FU.put("خمسه و عشرة", 15);
        FU.put("خمسه و عشره", 15);
        FU.put("خمسه وأربعون", 45);
        FU.put("خمسه وأربعين", 45);
        FU.put("خمسه واربعون", 45);
        FU.put("خمسه واربعين", 45);
        FU.put("خمسه وتسعون", 95);
        FU.put("خمسه وتسعين", 95);
        FU.put("خمسه وثلاثون", 35);
        FU.put("خمسه وثلاثين", 35);
        FU.put("خمسه وثمانون", 85);
        FU.put("خمسه وثمانين", 85);
        FU.put("خمسه وخمسون", 55);
        FU.put("خمسه وخمسين", 55);
        FU.put("خمسه وسبعون", 75);
        FU.put("خمسه وسبعين", 75);
        FU.put("خمسه وستون", 65);
        FU.put("خمسه وستين", 65);
        FU.put("خمسه وعشرة", 15);
        FU.put("خمسه وعشره", 15);
        FU.put("خمسه وعشرون", 25);
        FU.put("خمسه وعشرين", 25);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}