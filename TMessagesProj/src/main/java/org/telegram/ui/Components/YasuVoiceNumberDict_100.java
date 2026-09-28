package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_100 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        FU.put("اثنان", 2);
        FU.put("اثنان و عشرة", 12);
        FU.put("اثنان و عشره", 12);
        FU.put("اثنان وأربعون", 42);
        FU.put("اثنان وأربعين", 42);
        FU.put("اثنان واربعون", 42);
        FU.put("اثنان واربعين", 42);
        FU.put("اثنان وتسعون", 92);
        FU.put("اثنان وتسعين", 92);
        FU.put("اثنان وثلاثون", 32);
        FU.put("اثنان وثلاثين", 32);
        FU.put("اثنان وثمانون", 82);
        FU.put("اثنان وثمانين", 82);
        FU.put("اثنان وخمسون", 52);
        FU.put("اثنان وخمسين", 52);
        FU.put("اثنان وسبعون", 72);
        FU.put("اثنان وسبعين", 72);
        FU.put("اثنان وستون", 62);
        FU.put("اثنان وستين", 62);
        FU.put("اثنان وعشرة", 12);
        FU.put("اثنان وعشره", 12);
        FU.put("اثنان وعشرون", 22);
        FU.put("اثنان وعشرين", 22);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}