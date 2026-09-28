package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_105 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("اثنينه", 2);
        IQ.put("اثنينه و عشرة", 12);
        IQ.put("اثنينه و عشره", 12);
        IQ.put("اثنينه وأربعين", 42);
        IQ.put("اثنينه واربعين", 42);
        IQ.put("اثنينه وتسعين", 92);
        IQ.put("اثنينه وثلاثين", 32);
        IQ.put("اثنينه وثمانين", 82);
        IQ.put("اثنينه وثمنين", 82);
        IQ.put("اثنينه وخمسين", 52);
        IQ.put("اثنينه وسبعين", 72);
        IQ.put("اثنينه وستين", 62);
        IQ.put("اثنينه وعشرة", 12);
        IQ.put("اثنينه وعشره", 12);
        IQ.put("اثنينه وعشرين", 22);
        IQ.put("اثنينه وكلاثين", 32);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}