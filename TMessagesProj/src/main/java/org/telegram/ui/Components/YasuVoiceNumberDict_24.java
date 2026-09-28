package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_24 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("أربعة", 4);
        IQ.put("أربعة عشر", 14);
        IQ.put("أربعة و عشرة", 14);
        IQ.put("أربعة و عشره", 14);
        IQ.put("أربعة وأربعين", 44);
        IQ.put("أربعة وتسعين", 94);
        IQ.put("أربعة وثلاثين", 34);
        IQ.put("أربعة وثمانين", 84);
        IQ.put("أربعة وثمنين", 84);
        IQ.put("أربعة وخمسين", 54);
        IQ.put("أربعة وسبعين", 74);
        IQ.put("أربعة وستين", 64);
        IQ.put("أربعة وعشرة", 14);
        IQ.put("أربعة وعشره", 14);
        IQ.put("أربعة وعشرين", 24);
        IQ.put("أربعة وكلاثين", 34);
        FU.put("أربعة", 4);
        FU.put("أربعة عشر", 14);
        FU.put("أربعة و عشرة", 14);
        FU.put("أربعة و عشره", 14);
        FU.put("أربعة وأربعون", 44);
        FU.put("أربعة وأربعين", 44);
        FU.put("أربعة وتسعون", 94);
        FU.put("أربعة وتسعين", 94);
        FU.put("أربعة وثلاثون", 34);
        FU.put("أربعة وثلاثين", 34);
        FU.put("أربعة وثمانون", 84);
        FU.put("أربعة وثمانين", 84);
        FU.put("أربعة وخمسون", 54);
        FU.put("أربعة وخمسين", 54);
        FU.put("أربعة وسبعون", 74);
        FU.put("أربعة وسبعين", 74);
        FU.put("أربعة وستون", 64);
        FU.put("أربعة وستين", 64);
        FU.put("أربعة وعشرة", 14);
        FU.put("أربعة وعشره", 14);
        FU.put("أربعة وعشرون", 24);
        FU.put("أربعة وعشرين", 24);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}