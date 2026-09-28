package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_96 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("أربعه", 4);
        IQ.put("أربعه عشر", 14);
        IQ.put("أربعه و عشرة", 14);
        IQ.put("أربعه و عشره", 14);
        IQ.put("أربعه وأربعين", 44);
        IQ.put("أربعه وتسعين", 94);
        IQ.put("أربعه وثلاثين", 34);
        IQ.put("أربعه وثمانين", 84);
        IQ.put("أربعه وثمنين", 84);
        IQ.put("أربعه وخمسين", 54);
        IQ.put("أربعه وسبعين", 74);
        IQ.put("أربعه وستين", 64);
        IQ.put("أربعه وعشرة", 14);
        IQ.put("أربعه وعشره", 14);
        IQ.put("أربعه وعشرين", 24);
        IQ.put("أربعه وكلاثين", 34);
        FU.put("أربعه", 4);
        FU.put("أربعه عشر", 14);
        FU.put("أربعه و عشرة", 14);
        FU.put("أربعه و عشره", 14);
        FU.put("أربعه وأربعون", 44);
        FU.put("أربعه وأربعين", 44);
        FU.put("أربعه وتسعون", 94);
        FU.put("أربعه وتسعين", 94);
        FU.put("أربعه وثلاثون", 34);
        FU.put("أربعه وثلاثين", 34);
        FU.put("أربعه وثمانون", 84);
        FU.put("أربعه وثمانين", 84);
        FU.put("أربعه وخمسون", 54);
        FU.put("أربعه وخمسين", 54);
        FU.put("أربعه وسبعون", 74);
        FU.put("أربعه وسبعين", 74);
        FU.put("أربعه وستون", 64);
        FU.put("أربعه وستين", 64);
        FU.put("أربعه وعشرة", 14);
        FU.put("أربعه وعشره", 14);
        FU.put("أربعه وعشرون", 24);
        FU.put("أربعه وعشرين", 24);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}