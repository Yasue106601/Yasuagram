package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_133 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("اربعة", 4);
        IQ.put("اربعة عشر", 14);
        IQ.put("اربعة و عشرة", 14);
        IQ.put("اربعة و عشره", 14);
        IQ.put("اربعة واربعين", 44);
        IQ.put("اربعة وتسعين", 94);
        IQ.put("اربعة وثلاثين", 34);
        IQ.put("اربعة وثمانين", 84);
        IQ.put("اربعة وثمنين", 84);
        IQ.put("اربعة وخمسين", 54);
        IQ.put("اربعة وسبعين", 74);
        IQ.put("اربعة وستين", 64);
        IQ.put("اربعة وعشرة", 14);
        IQ.put("اربعة وعشره", 14);
        IQ.put("اربعة وعشرين", 24);
        IQ.put("اربعة وكلاثين", 34);
        FU.put("اربعة", 4);
        FU.put("اربعة عشر", 14);
        FU.put("اربعة و عشرة", 14);
        FU.put("اربعة و عشره", 14);
        FU.put("اربعة واربعون", 44);
        FU.put("اربعة واربعين", 44);
        FU.put("اربعة وتسعون", 94);
        FU.put("اربعة وتسعين", 94);
        FU.put("اربعة وثلاثون", 34);
        FU.put("اربعة وثلاثين", 34);
        FU.put("اربعة وثمانون", 84);
        FU.put("اربعة وثمانين", 84);
        FU.put("اربعة وخمسون", 54);
        FU.put("اربعة وخمسين", 54);
        FU.put("اربعة وسبعون", 74);
        FU.put("اربعة وسبعين", 74);
        FU.put("اربعة وستون", 64);
        FU.put("اربعة وستين", 64);
        FU.put("اربعة وعشرة", 14);
        FU.put("اربعة وعشره", 14);
        FU.put("اربعة وعشرون", 24);
        FU.put("اربعة وعشرين", 24);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}