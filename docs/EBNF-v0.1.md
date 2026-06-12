```ebnf
program
    ::= opening top_level_item* ending EOF
    ;


opening
    ::= fairy_opening "," country_intro "," kingdom_intro "."
    ;

fairy_opening
    ::= "היה" "הייתה"
    |   "הָיֹה" "הָיָה"
    |   "הָיֹה" "הָיְתָה"
    ;

country_intro
    ::= "בארץ" distance_phrase "ושמה" country_name
    ;

distance_phrase
    ::= "רחוקה" "רחוקה"
    |   "רחוקה"
    ;

kingdom_intro
    ::= "ממלכה" kingdom_epithet? "ושמה" kingdom_name
    ;

kingdom_epithet
    ::= "קטנה"
    |   "גדולה"
    |   "נסתרת"
    |   "גלויה"
    |   "עתיקה"
    ;

ending
    ::= "וכך" "תם" "סיפורה" "של" "ממלכת" kingdom_name "."
    ;


top_level_item
    ::= import_statement
    |   declaration
    |   scene
    |   statement
    ;


import_statement
    ::= ancient_kingdom_import
    |   local_kingdom_import
    |   foreign_country_import
    ;

ancient_kingdom_import
    ::= "מן" "הממלכה" "העתיקה"
        "הגיע" local_alias "ושמו" external_symbol "."
    |   "מן" "הממלכה" "העתיקה"
        "הגיעה" local_alias "ושמה" external_symbol "."
    ;

local_kingdom_import
    ::= "מן" "ממלכת" kingdom_name
        "הגיע" exported_symbol "ושמו" local_alias "."
    |   "מן" "ממלכת" kingdom_name
        "הגיעה" exported_symbol "ושמה" local_alias "."
    ;

foreign_country_import
    ::= "מן" country_name "ומממלכת" kingdom_name
        "הגיע" exported_symbol "ושמו" local_alias "."
    |   "מן" country_name "ומממלכת" kingdom_name
        "הגיעה" exported_symbol "ושמה" local_alias "."
    ;

declaration
    ::= boolean_declaration
    |   text_declaration
    |   count_declaration
    ;

boolean_declaration
    ::= definite_object_name "היה" boolean_masc "."
    |   definite_object_name "הייתה" boolean_fem "."
    ;

boolean_masc
    ::= "פתוח"
    |   "סגור"
    |   "דולק"
    |   "כבוי"
    ;

boolean_fem
    ::= "פתוחה"
    |   "סגורה"
    |   "דולקת"
    |   "כבויה"
    ;

text_declaration
    ::= "על" definite_object_name "נכתב" string "."
    |   in_object_name "נכתב" string "."
    ;

count_declaration
    ::= in_container_name "נחו" count_amount "."
    |   in_container_name "היו" count_amount "."
    |   "על" definite_container_name "עמדו" count_amount "."
    ;

count_amount
    ::= number unit_plural
    ;


scene
    ::= scene_intro ":" statement* scene_end?
    ;

scene_intro
    ::= time_phrase
    |   place_phrase
    ;

time_phrase
    ::= "בלילה" word* "אחד"
    |   "בבוקר" word*
    |   "עם" "עלות" "השחר"
    ;

place_phrase
    ::= "לפני" definite_object_name
    |   in_place_name
    ;

scene_end
    ::= "תם" "הפרק" "."
    ;


statement
    ::= boolean_change
    |   count_change
    |   text_change
    |   speech
    |   loop_statement
    |   condition_statement
    |   narrative_event
    ;


boolean_change
    ::= definite_object_name "נפתח" "."
    |   definite_object_name "נסגר" "."
    |   definite_object_name "נדלק" "."
    |   definite_object_name "כבה" "."
    ;

count_change
    ::= "נוסף" to_container_name unit_singular "אחד" "."
    |   "נוסף" to_container_name unit_singular number "."
    |   "נוספו" to_container_name number unit_plural "."
    |   "נגרע" from_container_name unit_singular "אחד" "."
    |   "נגרע" from_container_name unit_singular number "."
    |   "נגרעו" from_container_name number unit_plural "."
    ;

text_change
    ::= "על" definite_object_name "נכתב" "מעתה" string "."
    |   in_object_name "נכתב" "מעתה" string "."
    ;


speech
    ::= definite_alias_name speech_verb string "."
    |   definite_alias_name speech_verb "את" written_text_reference "."
    ;

speech_verb
    ::= "קרא"
    |   "קראה"
    |   "סיפר"
    |   "סיפרה"
    ;

written_text_reference
    ::= "הכתוב" "שעל" definite_object_name
    |   "הכתוב" "שב" definite_object_name
    ;


loop_statement
    ::= loop_intro ":" statement* loop_end
    ;

loop_intro
    ::= "שוב" "ושוב" "," "כל" "עוד" condition
    |   "יום" "אחר" "יום" "," "כל" "עוד" condition
    ;

loop_end
    ::= "כך" "חזר" "הדבר" "."
    ;


condition_statement
    ::= condition_intro ":" statement* condition_end
    |   condition_intro ":" statement* else_clause condition_end
    ;

condition_intro
    ::= "כאשר" condition
    |   "אם" condition
    ;

else_clause
    ::= "ואם" "לא" ":" statement*
    |   "אחרת" ":" statement*
    ;

condition_end
    ::= "תם" "הדבר" "."
    ;


condition
    ::= count_condition
    |   boolean_condition
    |   text_condition
    |   utterance_condition
    ;

count_condition
    ::= in_container_name "נחו" count_comparison
    |   in_container_name "היו" count_comparison
    ;

count_comparison
    ::= "פחות" mi_prefix number unit_plural
    |   "יותר" mi_prefix number unit_plural
    |   "לפחות" number unit_plural
    |   "בדיוק" number unit_plural
    |   number unit_plural
    ;

boolean_condition
    ::= definite_object_name "היה" boolean_masc
    |   definite_object_name "הייתה" boolean_fem
    ;

text_condition
    ::= written_text_reference "היה" string
    ;

utterance_condition
    ::= utterance_verb actor_name string
    |   definite_actor_name utterance_verb string
    ;

utterance_verb
    ::= "אמר"
    |   "אמרה"
    ;


narrative_event
    ::= arrival_event
    |   sound_event
    |   hiding_event
    |   knowing_event
    |   question_event
    |   gaze_event
    |   silence_event
    ;

arrival_event
    ::= "אל" place_name "נכנס" actor_name "."
    |   "אל" place_name "נכנסה" actor_name "."
    ;

sound_event
    ::= "נשמעו" count_amount in_object_name "."
    ;

hiding_event
    ::= actor_reference "הסתיר" item_phrase "מתחת" to_object_name "."
    |   actor_reference "הסתירה" item_phrase "מתחת" to_object_name "."
    ;

knowing_event
    ::= actor_reference "ידע" "כי" truth_phrase "."
    |   actor_reference "ידעה" "כי" truth_phrase "."
    ;

question_event
    ::= actor_reference "שאל" "את" definite_actor_name question_phrase "."
    |   actor_reference "שאלה" "את" definite_actor_name question_phrase "."
    ;

gaze_event
    ::= actor_reference "הביט" in_actor_name "."
    |   actor_reference "הביטה" in_actor_name "."
    ;

silence_event
    ::= actor_reference "שתק" "."
    |   actor_reference "שתקה" "."
    ;

actor_reference
    ::= definite_actor_name
    |   "הוא"
    |   "היא"
    ;

item_phrase
    ::= object_name
    |   object_name "אחד"
    |   object_name "אחת"
    ;

truth_phrase
    ::= "אמת" "בפי" definite_actor_name
    |   "שקר" "בפי" definite_actor_name
    ;

question_phrase
    ::= word+
    ;


country_name
    ::= identifier
    ;

kingdom_name
    ::= identifier
    ;

object_name
    ::= identifier
    ;

container_name
    ::= identifier
    ;

place_name
    ::= identifier
    ;

actor_name
    ::= identifier
    ;

local_alias
    ::= identifier
    ;

exported_symbol
    ::= identifier
    |   external_symbol
    ;

external_symbol
    ::= ASCII_IDENTIFIER
    ;


definite_object_name
    ::= object_name
    |   ha_prefix object_name
    ;

definite_container_name
    ::= container_name
    |   ha_prefix container_name
    ;

definite_actor_name
    ::= actor_name
    |   ha_prefix actor_name
    ;

definite_alias_name
    ::= local_alias
    |   ha_prefix local_alias
    ;


in_container_name
    ::= bet_prefix container_name
    ;

to_container_name
    ::= lamed_prefix container_name
    ;

from_container_name
    ::= mem_prefix container_name
    ;

in_object_name
    ::= bet_prefix object_name
    ;

to_object_name
    ::= lamed_prefix object_name
    ;

in_place_name
    ::= bet_prefix place_name
    ;

in_actor_name
    ::= bet_prefix actor_name
    ;

mi_prefix
    ::= "מ־"
    |   "מ"
    ;


identifier
    ::= HEBREW_WORD ("־" HEBREW_WORD)*
    ;

ha_prefix
    ::= "ה"
    ;

bet_prefix
    ::= "ב"
    ;

lamed_prefix
    ::= "ל"
    ;

mem_prefix
    ::= "מ"
    ;


number
    ::= DIGITS
    |   hebrew_number
    ;

hebrew_number
    ::= "אפס"
    |   "אחד"
    |   "אחת"
    |   "שניים"
    |   "שתיים"
    |   "שלושה"
    |   "שלוש"
    |   "ארבעה"
    |   "ארבע"
    |   "חמישה"
    |   "חמש"
    |   "שישה"
    |   "שש"
    |   "שבעה"
    |   "שבע"
    |   "שמונה"
    |   "תשעה"
    |   "תשע"
    |   "עשרה"
    |   "עשר"
    |   "עשרים"
    ;

unit_singular
    ::= HEBREW_WORD
    ;

unit_plural
    ::= HEBREW_WORD
    ;

word
    ::= HEBREW_WORD
    ;

string
    ::= STRING_LITERAL
    ;
```
