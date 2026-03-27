(define
    (problem harmony)
    (:domain harmony)
    (:requirements :ordered-goals)

    (:objects
        detections/audio/vad detections/vision/humans/person_0 shadow_objects/firefly - entity
        RECEPTION_DESK_loc TABLE_B_loc front_of_gifts_1_loc front_of_reception_desk_loc man_in_shop_rune_loc - location
        RECEPTION_DESK TABLE_B front_of_gifts_1 front_of_reception_desk man_in_shop_rune - rune
        find_desired_location_key find_desired_location_value hello_i_am_miroki - string
    )

    (:init
        (= (focused unknown_human) me)
        (= (focused me) unknown_human)
        (= (is_carrying_too_heavy_payload) false)
        (= (is_clicked RECEPTION_DESK) false)
        (= (is_clicked TABLE_B) false)
        (= (is_clicked front_of_gifts_1) false)
        (= (is_clicked front_of_reception_desk) false)
        (= (is_clicked man_in_shop_rune) false)
        (is_detected unknown_human)
        (= (is_double_clicked RECEPTION_DESK) false)
        (= (is_double_clicked TABLE_B) false)
        (= (is_double_clicked front_of_gifts_1) false)
        (= (is_double_clicked front_of_reception_desk) false)
        (= (is_double_clicked man_in_shop_rune) false)
        (= (is_in_front unknown_human) true)
        (= (is_long_pressed RECEPTION_DESK) false)
        (= (is_long_pressed TABLE_B) false)
        (= (is_long_pressed front_of_gifts_1) false)
        (= (is_long_pressed front_of_reception_desk) false)
        (= (is_long_pressed man_in_shop_rune) false)
        (= (is_nearby unknown_human) true)
        (= (is_touched both_ears) false)
        (= (is_touched left_ear) false)
        (= (is_touched right_ear) false)
        (= (last_time_appeared unknown_human) 1774533486.873965)
        (= (last_time_seen unknown_human) 1774533734.2120874)
        (= (location_of RECEPTION_DESK) RECEPTION_DESK_loc)
        (= (location_of TABLE_B) TABLE_B_loc)
        (= (location_of front_of_gifts_1) front_of_gifts_1_loc)
        (= (location_of front_of_reception_desk) front_of_reception_desk_loc)
        (= (location_of man_in_shop_rune) man_in_shop_rune_loc)
        (= (runtime_prompt find_desired_location_key) find_desired_location_value)
        (= (sees_someone) true)
        (told hello_i_am_miroki me unknown_human)
        (= (touches_count both_ears) 0)
        (= (touches_count left_ear) 0)
        (= (touches_count right_ear) 0)
        (= (wanted_location_of unknown_human) front_of_gifts_1_loc)
    )

    (:ordered-goals
        :effect-between-goals
            (forall (?resource - entity) (not (resource_has_been_taken ?resource)))

        :goals
            (ordered-list
              (users_at_desired_location)
 ;               (forall (?user_param - user) (imply
 ;           (not (= (immutable---wanted_location_of ?user_param) undefined))
 ;           (= (location_of ?user_param) (wanted_location_of ?user_param))
 ;       ))
            )
    )

)
