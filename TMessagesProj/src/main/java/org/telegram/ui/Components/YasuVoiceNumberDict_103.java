package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_103 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        FU.put("اثني", 2);
        FU.put("اثني عشر", 12);
        FU.put("اثني و عشرة", 12);
        FU.put("اثني و عشره", 12);
        FU.put("اثني وأربعون", 42);
        FU.put("اثني وأربعين", 42);
        FU.put("اثني واربعون", 42);
        FU.put("اثني واربعين", 42);
        FU.put("اثني وتسعون", 92);
        FU.put("اثني وتسعين", 92);
        FU.put("اثني وثلاثون", 32);
        FU.put("اثني وثلاثين", 32);
        FU.put("اثني وثمانون", 82);
        FU.put("اثني وثمانين", 82);
        FU.put("اثني وخمسون", 52);
        FU.put("اثني وخمسين", 52);
        FU.put("اثني وسبعون", 72);
        FU.put("اثني وسبعين", 72);
        FU.put("اثني وستون", 62);
        FU.put("اثني وستين", 62);
        FU.put("اثني وعشرة", 12);
        FU.put("اثني وعشره", 12);
        FU.put("اثني وعشرون", 22);
        FU.put("اثني وعشرين", 22);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}