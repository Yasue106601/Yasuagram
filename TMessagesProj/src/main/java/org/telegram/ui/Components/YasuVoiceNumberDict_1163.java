package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_1163 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        FU.put("واحدا", 1);
        FU.put("واحدا و عشرة", 11);
        FU.put("واحدا و عشره", 11);
        FU.put("واحدا وأربعون", 41);
        FU.put("واحدا وأربعين", 41);
        FU.put("واحدا واربعون", 41);
        FU.put("واحدا واربعين", 41);
        FU.put("واحدا وتسعون", 91);
        FU.put("واحدا وتسعين", 91);
        FU.put("واحدا وثلاثون", 31);
        FU.put("واحدا وثلاثين", 31);
        FU.put("واحدا وثمانون", 81);
        FU.put("واحدا وثمانين", 81);
        FU.put("واحدا وخمسون", 51);
        FU.put("واحدا وخمسين", 51);
        FU.put("واحدا وسبعون", 71);
        FU.put("واحدا وسبعين", 71);
        FU.put("واحدا وستون", 61);
        FU.put("واحدا وستين", 61);
        FU.put("واحدا وعشرة", 11);
        FU.put("واحدا وعشره", 11);
        FU.put("واحدا وعشرون", 21);
        FU.put("واحدا وعشرين", 21);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}