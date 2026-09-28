package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_99 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("اثنا عشر", 12);
        FU.put("اثنا", 2);
        FU.put("اثنا عشر", 12);
        FU.put("اثنا عشره", 12);
        FU.put("اثنا و عشرة", 12);
        FU.put("اثنا و عشره", 12);
        FU.put("اثنا وأربعون", 42);
        FU.put("اثنا وأربعين", 42);
        FU.put("اثنا واربعون", 42);
        FU.put("اثنا واربعين", 42);
        FU.put("اثنا وتسعون", 92);
        FU.put("اثنا وتسعين", 92);
        FU.put("اثنا وثلاثون", 32);
        FU.put("اثنا وثلاثين", 32);
        FU.put("اثنا وثمانون", 82);
        FU.put("اثنا وثمانين", 82);
        FU.put("اثنا وخمسون", 52);
        FU.put("اثنا وخمسين", 52);
        FU.put("اثنا وسبعون", 72);
        FU.put("اثنا وسبعين", 72);
        FU.put("اثنا وستون", 62);
        FU.put("اثنا وستين", 62);
        FU.put("اثنا وعشرة", 12);
        FU.put("اثنا وعشره", 12);
        FU.put("اثنا وعشرون", 22);
        FU.put("اثنا وعشرين", 22);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}