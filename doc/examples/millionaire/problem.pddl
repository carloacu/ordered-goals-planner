(define (problem becoming-rich)

  (:domain millionaire)

  (:objects
    alice - agent
  )

  (:init
    (= (money  alice) 7000)
    (= (health alice) 100)
    (= (hours  alice) 0)
  )

  (:goal (and
    (>= (money  alice) 1000000)
    (>= (health alice) 80)
  ))

  (:metric minimize (total-cost))
)