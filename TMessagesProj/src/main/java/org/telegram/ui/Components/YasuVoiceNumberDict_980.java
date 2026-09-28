package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_980 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("كلاثه", 3);
        IQ.put("كلاثه و عشرة", 13);
        IQ.put("كلاثه و عشره", 13);
        IQ.put("كلاثه وأربعين", 43);
        IQ.put("كلاثه واربعين", 43);
        IQ.put("كلاثه وتسعين", 93);
        IQ.put("كلاثه وثلاثين", 33);
        IQ.put("كلاثه وثمانين", 83);
        IQ.put("كلاثه وثمنين", 83);
        IQ.put("كلاثه وخمسين", 53);
        IQ.put("كلاثه وسبعين", 73);
        IQ.put("كلاثه وستين", 63);
        IQ.put("كلاثه وعشرة", 13);
        IQ.put("كلاثه وعشره", 13);
        IQ.put("كلاثه وعشرين", 23);
        IQ.put("كلاثه وكلاثين", 33);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}