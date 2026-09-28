package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_104 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("اثنين", 2);
        IQ.put("اثنين و عشرة", 12);
        IQ.put("اثنين و عشره", 12);
        IQ.put("اثنين وأربعين", 42);
        IQ.put("اثنين واربعين", 42);
        IQ.put("اثنين وتسعين", 92);
        IQ.put("اثنين وثلاثين", 32);
        IQ.put("اثنين وثمانين", 82);
        IQ.put("اثنين وثمنين", 82);
        IQ.put("اثنين وخمسين", 52);
        IQ.put("اثنين وسبعين", 72);
        IQ.put("اثنين وستين", 62);
        IQ.put("اثنين وعشرة", 12);
        IQ.put("اثنين وعشره", 12);
        IQ.put("اثنين وعشرين", 22);
        IQ.put("اثنين وكلاثين", 32);
        FU.put("اثنين", 2);
        FU.put("اثنين و عشرة", 12);
        FU.put("اثنين و عشره", 12);
        FU.put("اثنين وأربعون", 42);
        FU.put("اثنين وأربعين", 42);
        FU.put("اثنين واربعون", 42);
        FU.put("اثنين واربعين", 42);
        FU.put("اثنين وتسعون", 92);
        FU.put("اثنين وتسعين", 92);
        FU.put("اثنين وثلاثون", 32);
        FU.put("اثنين وثلاثين", 32);
        FU.put("اثنين وثمانون", 82);
        FU.put("اثنين وثمانين", 82);
        FU.put("اثنين وخمسون", 52);
        FU.put("اثنين وخمسين", 52);
        FU.put("اثنين وسبعون", 72);
        FU.put("اثنين وسبعين", 72);
        FU.put("اثنين وستون", 62);
        FU.put("اثنين وستين", 62);
        FU.put("اثنين وعشرة", 12);
        FU.put("اثنين وعشره", 12);
        FU.put("اثنين وعشرون", 22);
        FU.put("اثنين وعشرين", 22);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}