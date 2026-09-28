package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_205 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("اربعه", 4);
        IQ.put("اربعه عشر", 14);
        IQ.put("اربعه و عشرة", 14);
        IQ.put("اربعه و عشره", 14);
        IQ.put("اربعه واربعين", 44);
        IQ.put("اربعه وتسعين", 94);
        IQ.put("اربعه وثلاثين", 34);
        IQ.put("اربعه وثمانين", 84);
        IQ.put("اربعه وثمنين", 84);
        IQ.put("اربعه وخمسين", 54);
        IQ.put("اربعه وسبعين", 74);
        IQ.put("اربعه وستين", 64);
        IQ.put("اربعه وعشرة", 14);
        IQ.put("اربعه وعشره", 14);
        IQ.put("اربعه وعشرين", 24);
        IQ.put("اربعه وكلاثين", 34);
        FU.put("اربعه", 4);
        FU.put("اربعه عشر", 14);
        FU.put("اربعه و عشرة", 14);
        FU.put("اربعه و عشره", 14);
        FU.put("اربعه واربعون", 44);
        FU.put("اربعه واربعين", 44);
        FU.put("اربعه وتسعون", 94);
        FU.put("اربعه وتسعين", 94);
        FU.put("اربعه وثلاثون", 34);
        FU.put("اربعه وثلاثين", 34);
        FU.put("اربعه وثمانون", 84);
        FU.put("اربعه وثمانين", 84);
        FU.put("اربعه وخمسون", 54);
        FU.put("اربعه وخمسين", 54);
        FU.put("اربعه وسبعون", 74);
        FU.put("اربعه وسبعين", 74);
        FU.put("اربعه وستون", 64);
        FU.put("اربعه وستين", 64);
        FU.put("اربعه وعشرة", 14);
        FU.put("اربعه وعشره", 14);
        FU.put("اربعه وعشرون", 24);
        FU.put("اربعه وعشرين", 24);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}