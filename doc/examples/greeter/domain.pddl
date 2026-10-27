(define
(domain harmony)
(:requirements :strips :typing :derived-predicates)

(:types
  boolean string location physical_object social_agent robot_part battery_status type_of_click position3_d pose3_d travel_status travel_failure - entity
  physical_agent rune - physical_object
  user robot - physical_agent
  enchanted_object enchanted_location enchanted_user enchanted_action - rune
  single_handle_object cart dual_handle_object charger - enchanted_object
  handle basket - single_handle_object
  tray tablet_holder_dual_handle - dual_handle_object
  deposit_zone - enchanted_location
  arm ear neck - robot_part
)

(:constants
  true false - boolean
  me - robot
  my_location - location
  potential_human - user
  unknown_handle generic_deposit_zone - enchanted_object
  interactive_status asr gaze listening voice mood_animation - entity
  left_arm right_arm - arm
  both_ears left_ear right_ear - ear
  face ball - robot_part
  neck_roll neck_pitch neck_yaw - neck
  simple_click double_click long_press - type_of_click
  battery_high battery_medium battery_low battery_critical - battery_status
  unknown_tray - tray
  stopped executing arrived cancelled failed unknown - travel_status
  localization_lost collision no_path_to_target out_of_bounds path_execution_failed - travel_failure
)

(:predicates
  (resource_taken ?resource - entity)
  (resource_has_been_taken ?resource - entity)
  (employee_informed_about_guest ?enchanter_name - string ?guest - (either physical_agent social_agent))
  (engages ?initiator - (either physical_agent social_agent) ?recipient - (either physical_agent social_agent))
  (has_told ?text - string ?from_agent - (either physical_agent social_agent) ?to_agent - (either physical_agent social_agent))
  (holds ?r - robot ?o - physical_object)
  (is_detected ?po - physical_object)
  (is_looking_at_a_human)
  (is_visible ?po - physical_object)
)

(:functions
  (acts_through ?agent - social_agent) - user
  (attention_preference ?a - physical_agent) - physical_object
  (battery_level) - battery_status
  (hand_holds ?a - arm) - physical_object
  (human_position ?user - enchanted_user) - position3_d
  (is_carrying_too_heavy_payload) - boolean
  (is_clicked ?r - rune) - boolean
  (is_double_clicked ?r - rune) - boolean
  (is_in_front ?o - physical_object) - boolean
  (is_localization_lost_of ?agent - physical_agent) - boolean
  (is_long_pressed ?r - rune) - boolean
  (is_nearby ?o - physical_object) - boolean
  (is_touched ?p - robot_part) - boolean
  (location_of ?o - (either physical_object social_agent)) - location
  (logistic_rune_target_location ?rune - rune) - location
  (logistic_rune_target_object ?rune - rune) - physical_object
  (looking_at ?a - physical_agent) - physical_object
  (pose_of ?location - location) - pose3_d
  (rune_associated_to_user ?rune - enchanted_user) - user
  (rune_informations ?r - rune) - boolean
  (runtime_prompt ?key - string) - string
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
)
(:derived (any_user_is_told ?text_to_say - string)
  (exists (?user_param - user) (has_told ?text_to_say me ?user_param))
)

(:derived (i_engage_any_user)
  (exists (?user_param - user) (engages me ?user_param))
)

(:derived (i_have_any_user_detected)
  (exists (?user_param - user) (is_detected ?user_param))
)

(:derived (i_see_any_user)
  (exists (?user_param - user) (is_visible ?user_param))
)

(:derived (is_at_rune ?object - physical_object ?rune - rune)
  (= (location_of ?object) (location_of ?rune))
)

(:derived (robot_at ?location - location)
  (= (location_of me) ?location)
)

(:derived (user_at_desired_location)
  (forall (?user_param - user) (if (not (= (wanted_location_of ?user_param) undefined)) (= (location_of ?user_param) (wanted_location_of ?user_param))))
)

(:derived (user_is_told ?text_to_say - string ?receiver - physical_agent)
  (has_told ?text_to_say me ?receiver)
)

(:derived (visible_user_at_desired_location)
  (exists (?user_param - user) (and (is_visible ?user_param) (not (= (wanted_location_of ?user_param) undefined)) (= (location_of ?user_param) (wanted_location_of ?user_param))))
)

(:derived (visible_user_wants_to_be_at_a_location)
  (exists (?user_param - user) (and (is_visible ?user_param) (not (= (wanted_location_of ?user_param) undefined))))
)

(:event move_grasped_objects
  :parameters
    (?po - physical_object ?loc - location)

  :precondition
    (and (holds me ?po) (= (location_of me) ?loc))

  :effect
    (assign (location_of ?po) ?loc)
)

(:event if_left_hand_holds_then_holds
  :parameters
    (?po - physical_object)

  :precondition
    (= (hand_holds left_arm) ?po)

  :effect
    (holds me ?po)
)

(:event if_right_hand_holds_then_holds
  :parameters
    (?po - physical_object)

  :precondition
    (= (hand_holds right_arm) ?po)

  :effect
    (holds me ?po)
)

(:event if_not_hand_holds_then_not_holds
  :parameters
    (?po - physical_object)

  :precondition
    (and (not (= (hand_holds left_arm) ?po)) (not (= (hand_holds right_arm) ?po)))

  :effect
    (not (holds me ?po))
)



(:durative-action wait_for_resource
  :parameters
    (?resource_to_wait - entity)

  :duration (= ?duration 10)

  :effect
    (at end (not (resource_taken ?resource_to_wait)))
)

(:durative-action reset_attention_preference
  :duration (= ?duration 10)

  :effect
    (at end (assign (attention_preference me) undefined))
)

(:durative-action set_attention_preference_for
  :parameters
    (?target - physical_object)

  :duration (= ?duration 10)

  :condition
    (at start (is_visible ?target))

  :effect
    (at end (assign (attention_preference me) ?target))
)

(:durative-action inform_employee_about_guest_arrival
  :parameters
    (?employee_name - string ?guest - social_agent)

  :duration (= ?duration 10)

  :effect
    (at end (employee_informed_about_guest ?employee_name ?guest))
)

(:durative-action find_social_agent
  :parameters
    (?agent - social_agent)

  :duration (= ?duration 10)

  :condition
    (at start (and (not (resource_taken ball)) (not (resource_taken neck_roll)) (not (resource_taken neck_pitch)) (not (resource_taken neck_yaw))))

  :effect
    (and
    (at start (and (resource_taken ball) (resource_taken neck_roll) (resource_taken neck_pitch) (resource_taken neck_yaw)))
    (at end (and (engages me ?agent) (and (not (resource_taken ball)) (resource_has_been_taken ball) (not (resource_taken neck_roll)) (resource_has_been_taken neck_roll) (not (resource_taken neck_pitch)) (resource_has_been_taken neck_pitch) (not (resource_taken neck_yaw)) (resource_has_been_taken neck_yaw))))
    )
)

(:durative-action look_around_until_human_is_seen
  :duration (= ?duration 10)

  :condition
    (at start (and (not (resource_taken neck_roll)) (not (resource_taken neck_pitch)) (not (resource_taken neck_yaw))))

  :effect
    (and
    (at start (and (resource_taken neck_roll) (resource_taken neck_pitch) (resource_taken neck_yaw)))
    (at end (and (is_visible potential_human) (not (resource_taken neck_roll)) (resource_has_been_taken neck_roll) (not (resource_taken neck_pitch)) (resource_has_been_taken neck_pitch) (not (resource_taken neck_yaw)) (resource_has_been_taken neck_yaw)))
    )
)

(:durative-action tell_social_agent
  :parameters
    (?agent - social_agent ?something - string)

  :duration (= ?duration 10)

  :condition
    (at start (and (engages me ?agent) (not (resource_taken voice)) (not (resource_taken asr))))

  :effect
    (and
    (at start (and (resource_taken voice) (resource_taken asr)))
    (at end (and (has_told ?something me ?agent) (not (resource_taken voice)) (resource_has_been_taken voice) (not (resource_taken asr)) (resource_has_been_taken asr)))
    )
)

(:durative-action tell_with_reformulation
  :parameters
    (?receiver - physical_agent ?something - string)

  :duration (= ?duration 10)

  :condition
    (at start (and (is_visible ?receiver) (not (resource_taken voice)) (not (resource_taken asr))))

  :effect
    (and
    (at start (and (resource_taken voice) (resource_taken asr)))
    (at end (and (has_told ?something me ?receiver) (not (resource_taken voice)) (resource_has_been_taken voice) (not (resource_taken asr)) (resource_has_been_taken asr)))
    )
)

(:durative-action go_to_location
  :parameters
    (?location - location)

  :duration (= ?duration 10)

  :condition
    (at start (and (= (is_carrying_too_heavy_payload) false) (not (resource_taken ball))))

  :effect
    (and
    (at start (resource_taken ball))
    (at end (and (assign (location_of me) ?location) (not (resource_taken ball)) (resource_has_been_taken ball)))
    )
)

(:durative-action guide_social_agent_to
  :parameters
    (?agent - social_agent ?destination - location)

  :duration (= ?duration 20)

  :condition
    (at start (and (= (is_carrying_too_heavy_payload) false) (not (resource_taken ball))))

  :effect
    (and
    (at start (resource_taken ball))
    (at end (and (assign (location_of ?agent) ?destination) (not (engages me ?agent)) (and (not (resource_taken ball)) (resource_has_been_taken ball))))
    )
)

(:durative-action guide_user_to
  :parameters
    (?user - user ?destination - location)

  :duration (= ?duration 20)

  :condition
    (at start (and (= (is_carrying_too_heavy_payload) false) (is_visible ?user) (not (resource_taken ball))))

  :effect
    (and
    (at start (resource_taken ball))
    (at end (and (assign (location_of me) ?destination) (assign (location_of ?user) ?destination) (and (not (resource_taken ball)) (resource_has_been_taken ball))))
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


)