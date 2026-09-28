package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_1162 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("واحد", 1);
        IQ.put("واحد و عشرة", 11);
        IQ.put("واحد و عشره", 11);
        IQ.put("واحد وأربعين", 41);
        IQ.put("واحد واربعين", 41);
        IQ.put("واحد وتسعين", 91);
        IQ.put("واحد وثلاثين", 31);
        IQ.put("واحد وثمانين", 81);
        IQ.put("واحد وثمنين", 81);
        IQ.put("واحد وخمسين", 51);
        IQ.put("واحد وسبعين", 71);
        IQ.put("واحد وستين", 61);
        IQ.put("واحد وعشرة", 11);
        IQ.put("واحد وعشره", 11);
        IQ.put("واحد وعشرين", 21);
        IQ.put("واحد وكلاثين", 31);
        FU.put("واحد", 1);
        FU.put("واحد و عشرة", 11);
        FU.put("واحد و عشره", 11);
        FU.put("واحد وأربعون", 41);
        FU.put("واحد وأربعين", 41);
        FU.put("واحد واربعون", 41);
        FU.put("واحد واربعين", 41);
        FU.put("واحد وتسعون", 91);
        FU.put("واحد وتسعين", 91);
        FU.put("واحد وثلاثون", 31);
        FU.put("واحد وثلاثين", 31);
        FU.put("واحد وثمانون", 81);
        FU.put("واحد وثمانين", 81);
        FU.put("واحد وخمسون", 51);
        FU.put("واحد وخمسين", 51);
        FU.put("واحد وسبعون", 71);
        FU.put("واحد وسبعين", 71);
        FU.put("واحد وستون", 61);
        FU.put("واحد وستين", 61);
        FU.put("واحد وعشرة", 11);
        FU.put("واحد وعشره", 11);
        FU.put("واحد وعشرون", 21);
        FU.put("واحد وعشرين", 21);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}