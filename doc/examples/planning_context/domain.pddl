(define
    (domain harmony)
    (:requirements :derived-predicates :strips :typing)

    (:types
        boolean number string location physical_object robot_part battery_status type_of_click position3_d pose3_d travel_status travel_failure - entity
        physical_agent rune - physical_object
        user robot - physical_agent
        enchanted_object enchanted_location enchanted_user enchanted_action - rune
        handle basket cart tray charger - enchanted_object
        deposit_zone - enchanted_location
        arm ear neck - robot_part
    )

    (:constants
        left_arm right_arm - arm
        battery_critical battery_high battery_low battery_medium - battery_status
        false true - boolean
        both_ears left_ear right_ear - ear
        unknown_handle - enchanted_object
        asr gaze interactive_status listening mood_animation voice - entity
        my_location - location
        neck_pitch neck_roll neck_yaw - neck
        me - robot
        ball face - robot_part
        collision localization_lost no_path_to_target out_of_bounds path_execution_failed - travel_failure
        arrived cancelled executing failed stopped unknown - travel_status
        unknown_tray - tray
        double_click long_press simple_click - type_of_click
        unknown_human - user
    )

    (:predicates
        (engages ?initiator - physical_agent ?recipient - physical_agent)
        (holds ?r - robot ?o - physical_object)
        (is_detected ?po - physical_object)
        (resource_has_been_taken ?resource - entity)
        (resource_taken ?resource - entity)
        (told ?text - string ?from_agent - physical_agent ?to_agent - physical_agent)
        (immutable__engages ?initiator - physical_agent ?recipient - physical_agent)
        (immutable__holds ?r - robot ?o - physical_object)
        (immutable__is_detected ?po - physical_object)
        (immutable__resource_has_been_taken ?resource - entity)
        (immutable__resource_taken ?resource - entity)
        (immutable__told ?text - string ?from_agent - physical_agent ?to_agent - physical_agent)
    )

    (:functions
        (battery_level) - battery_status
        (focused ?a - physical_agent) - physical_object
        (hand_holds ?a - arm) - physical_object
        (human_position ?user - enchanted_user) - position3_d
        (is_carrying_too_heavy_payload) - boolean
        (is_clicked ?r - rune) - boolean
        (is_double_clicked ?r - rune) - boolean
        (is_in_front ?o - physical_object) - boolean
        (is_long_pressed ?r - rune) - boolean
        (is_nearby ?o - physical_object) - boolean
        (is_touched ?p - robot_part) - boolean
        (last_time_appeared ?o - physical_object) - number
        (last_time_seen ?o - physical_object) - number
        (location_of ?o - physical_object) - location
        (logistic_rune_target_location ?rune - rune) - location
        (logistic_rune_target_object ?rune - rune) - physical_object
        (pose_of ?location - location) - pose3_d
        (rune_associated_to_user ?rune - enchanted_user) - user
        (rune_informations ?r - rune) - boolean
        (runtime_prompt ?key - string) - string
        (sees_someone) - boolean
        (touches_count ?p - robot_part) - number
        (travel_direction_of ?agent - physical_agent) - string
        (travel_eta_of ?agent - physical_agent) - number
        (travel_failure_reason_of ?agent - physical_agent) - travel_failure
        (travel_goal_id_of ?agent - physical_agent) - string
        (travel_progress_of ?agent - physical_agent) - number
        (travel_remaining_distance_of ?agent - physical_agent) - number
        (travel_status_of ?agent - physical_agent) - travel_status
        (travel_velocity_of ?agent - physical_agent) - number
        (user_associated_to_rune ?user - user) - enchanted_user
        (wanted_location_of ?o - physical_object) - location
        (immutable__battery_level) - battery_status
        (immutable__focused ?a - physical_agent) - physical_object
        (immutable__hand_holds ?a - arm) - physical_object
        (immutable__human_position ?user - enchanted_user) - position3_d
        (immutable__is_carrying_too_heavy_payload) - boolean
        (immutable__is_clicked ?r - rune) - boolean
        (immutable__is_double_clicked ?r - rune) - boolean
        (immutable__is_in_front ?o - physical_object) - boolean
        (immutable__is_long_pressed ?r - rune) - boolean
        (immutable__is_nearby ?o - physical_object) - boolean
        (immutable__is_touched ?p - robot_part) - boolean
        (immutable__last_time_appeared ?o - physical_object) - number
        (immutable__last_time_seen ?o - physical_object) - number
        (immutable__location_of ?o - physical_object) - location
        (immutable__logistic_rune_target_location ?rune - rune) - location
        (immutable__logistic_rune_target_object ?rune - rune) - physical_object
        (immutable__pose_of ?location - location) - pose3_d
        (immutable__rune_associated_to_user ?rune - enchanted_user) - user
        (immutable__rune_informations ?r - rune) - boolean
        (immutable__runtime_prompt ?key - string) - string
        (immutable__sees_someone) - boolean
        (immutable__touches_count ?p - robot_part) - number
        (immutable__travel_direction_of ?agent - physical_agent) - string
        (immutable__travel_eta_of ?agent - physical_agent) - number
        (immutable__travel_failure_reason_of ?agent - physical_agent) - travel_failure
        (immutable__travel_goal_id_of ?agent - physical_agent) - string
        (immutable__travel_progress_of ?agent - physical_agent) - number
        (immutable__travel_remaining_distance_of ?agent - physical_agent) - number
        (immutable__travel_status_of ?agent - physical_agent) - travel_status
        (immutable__travel_velocity_of ?agent - physical_agent) - number
        (immutable__user_associated_to_rune ?user - user) - enchanted_user
        (immutable__wanted_location_of ?o - physical_object) - location
    )

    (:derived (any_user_is_told ?text_to_say - string)
        (exists (?user_param - user) (told ?text_to_say me ?user_param))
    )

    (:derived (focus_any_user)
        (exists (?user_param - user) (= (focused me) ?user_param))
    )

    (:derived (focused_any_user)
        (exists (?user_param - user) (= (focused me) ?user_param))
    )

    (:derived (focused_to ?user - user)
        (= (focused me) ?user)
    )

    (:derived (focused_user_at_desired_location)
        (exists (?user_param - user) (and
            (= (focused me) ?user_param)
            (not (= (wanted_location_of ?user_param) undefined))
            (= (location_of ?user_param) (wanted_location_of ?user_param))
        ))
    )

    (:derived (focused_user_wants_to_be_at_a_location)
        (exists (?user_param - user) (and
            (= (focused me) ?user_param)
            (not (= (wanted_location_of ?user_param) undefined))
        ))
    )

    (:derived (is_at_rune ?object - physical_object ?rune - rune)
        (= (location_of ?object) (location_of ?rune))
    )

    (:derived (robot_at ?location - location)
        (= (location_of me) ?location)
    )

    (:derived (robot_not_focused)
        (= (focused me) undefined)
    )

    (:derived (unfocus_from_user)
        (forall (?user_param - user) (not (= (focused me) ?user_param)))
    )

    (:derived (user_is_told ?text_to_say - string ?receiver - physical_agent)
        (told ?text_to_say me ?receiver)
    )

    (:derived (users_at_desired_location)
        (forall (?user_param - user) (imply
            (not (= (immutable__wanted_location_of ?user_param) undefined))
            (= (location_of ?user_param) (wanted_location_of ?user_param))
        ))
    )

    (:derived (immutable__any_user_is_told ?text_to_say - string)
        (exists (?user_param - user) (immutable__told ?text_to_say me ?user_param))
    )

    (:derived (immutable__focus_any_user)
        (exists (?user_param - user) (= (immutable__focused me) ?user_param))
    )

    (:derived (immutable__focused_any_user)
        (exists (?user_param - user) (= (immutable__focused me) ?user_param))
    )

    (:derived (immutable__focused_to ?user - user)
        (= (immutable__focused me) ?user)
    )

    (:derived (immutable__focused_user_at_desired_location)
        (exists (?user_param - user) (and
            (= (immutable__focused me) ?user_param)
            (not (= (immutable__wanted_location_of ?user_param) undefined))
            (= (immutable__location_of ?user_param) (immutable__wanted_location_of ?user_param))
        ))
    )

    (:derived (immutable__focused_user_wants_to_be_at_a_location)
        (exists (?user_param - user) (and
            (= (immutable__focused me) ?user_param)
            (not (= (immutable__wanted_location_of ?user_param) undefined))
        ))
    )

    (:derived (immutable__is_at_rune ?object - physical_object ?rune - rune)
        (= (immutable__location_of ?object) (immutable__location_of ?rune))
    )

    (:derived (immutable__robot_at ?location - location)
        (= (immutable__location_of me) ?location)
    )

    (:derived (immutable__robot_not_focused)
        (= (immutable__focused me) undefined)
    )

    (:derived (immutable__unfocus_from_user)
        (forall (?user_param - user) (not (= (immutable__focused me) ?user_param)))
    )

    (:derived (immutable__user_is_told ?text_to_say - string ?receiver - physical_agent)
        (immutable__told ?text_to_say me ?receiver)
    )

    (:derived (immutable__users_at_desired_location)
        (forall (?user_param - user) (imply
            (not (= (immutable__wanted_location_of ?user_param) undefined))
            (= (immutable__location_of ?user_param) (immutable__wanted_location_of ?user_param))
        ))
    )

    (:event if_left_hand_holds_then_holds

        :parameters
            (?po - physical_object)

        :precondition
            (= (hand_holds left_arm) ?po)

        :effect
            (holds me ?po)
    )

    (:event if_not_hand_holds_then_not_holds

        :parameters
            (?po - physical_object)

        :precondition
            (and
                (not (= (hand_holds left_arm) ?po))
                (not (= (hand_holds right_arm) ?po))
            )

        :effect
            (not (holds me ?po))
    )

    (:event if_right_hand_holds_then_holds

        :parameters
            (?po - physical_object)

        :precondition
            (= (hand_holds right_arm) ?po)

        :effect
            (holds me ?po)
    )

    (:event move_grasped_objects

        :parameters
            (?po - physical_object ?loc - location)

        :precondition
            (and
                (holds me ?po)
                (= (location_of me) ?loc)
            )

        :effect
            (assign (location_of ?po) ?loc)
    )

    (:durative-action focus_on
        :parameters
            (?object_of_attention - physical_object)

        :duration (= ?duration 10)

        :condition
            (at start (is_detected ?object_of_attention))

        :effect
            (at end (assign (focused me) ?object_of_attention))
    )

    (:durative-action go_to_location
        :parameters
            (?location - location)

        :duration (= ?duration 10)

        :condition
            (at start (not (resource_taken ball)))

        :effect
            (and
                (at start (resource_taken ball))
                (at end (assign (location_of me) ?location))
                (at end (not (resource_taken ball)))
                (at end (resource_has_been_taken ball))
            )
    )

    (:durative-action guide_user_to
        :parameters
            (?user - user ?destination - location)

        :duration (= ?duration 20)

        :condition
            (and
                (at start (is_detected ?user))
                (at start (not (resource_taken ball)))
            )

        :effect
            (and
                (at start (resource_taken ball))
                (at end (assign (location_of me) ?destination))
                (at end (assign (location_of ?user) ?destination))
                (at end (not (resource_taken ball)))
                (at end (resource_has_been_taken ball))
            )
    )

    (:durative-action look_around_until_human_is_seen
        :duration (= ?duration 10)

        :condition
            (and
                (at start (not (resource_taken neck_roll)))
                (at start (not (resource_taken neck_pitch)))
                (at start (not (resource_taken neck_yaw)))
            )

        :effect
            (and
                (at start (resource_taken neck_roll))
                (at start (resource_taken neck_pitch))
                (at start (resource_taken neck_yaw))
                (at end (is_detected unknown_human))
                (at end (assign (focused unknown_human) me))
                (at end (not (resource_taken neck_roll)))
                (at end (resource_has_been_taken neck_roll))
                (at end (not (resource_taken neck_pitch)))
                (at end (resource_has_been_taken neck_pitch))
                (at end (not (resource_taken neck_yaw)))
                (at end (resource_has_been_taken neck_yaw))
            )
    )

    (:durative-action remove_runtime_prompt
        :parameters
            (?key - string)

        :duration (= ?duration 10)

        :effect
            (at end (assign (runtime_prompt ?key) undefined))
    )

    (:durative-action set_runtime_prompt
        :parameters
            (?key - string ?value - string)

        :duration (= ?duration 10)

        :effect
            (at end (assign (runtime_prompt ?key) ?value))
    )

    (:durative-action tell_with_reformulation
        :parameters
            (?receiver - physical_agent ?something - string)

        :duration (= ?duration 10)

        :condition
            (and
                (at start (= (focused me) ?receiver))
                (at start (not (resource_taken voice)))
                (at start (not (resource_taken asr)))
            )

        :effect
            (and
                (at start (resource_taken voice))
                (at start (resource_taken asr))
                (at end (told ?something me ?receiver))
                (at end (not (resource_taken voice)))
                (at end (resource_has_been_taken voice))
                (at end (not (resource_taken asr)))
                (at end (resource_has_been_taken asr))
            )
    )

    (:durative-action un_focus
        :duration (= ?duration 10)

        :effect
            (at end (assign (focused me) undefined))
    )

    (:durative-action wait_for_resource
        :parameters
            (?resource_to_wait - entity)

        :duration (= ?duration 10)

        :effect
            (at end (not (resource_taken ?resource_to_wait)))
    )

)
