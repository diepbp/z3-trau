
(declare-fun in0 () String)

(assert (and (and (and (and (and (and (and (and (and (and (and (and (and (not (not (= (ite (str.contains (str.substr in0 0 (- (str.indexof in0 "@" 0) 0)) "+") 1 0) 0))) (not (not (= (ite (= (str.len (str.substr in0 0 (- (str.indexof in0 "@" 0) 0))) 0) 1 0) 0)))) (not (not (= (ite (str.contains (str.substr in0 (+ (str.indexof in0 "@" 0) 1) (- (str.len in0) (+ (str.indexof in0 "@" 0) 1))) "@") 1 0) 0)))) (not (not (= (ite (= (str.len (str.substr in0 (+ (str.indexof in0 "@" 0) 1) (- (str.len in0) (+ (str.indexof in0 "@" 0) 1)))) 0) 1 0) 0)))) (not (= (ite (str.contains in0 "@") 1 0) 0))) (not (not (= (ite (= (str.len in0) 0) 1 0) 0)))) (>= 0 0)) (>= (- (str.indexof in0 "@" 0) 0) 0)) (>= 0 0)) (>= (- (str.indexof in0 "@" 0) 0) 0)) (>= (+ (str.indexof in0 "@" 0) 1) 0)) (>= (- (str.len in0) (+ (str.indexof in0 "@" 0) 1)) 0)) (>= (+ (str.indexof in0 "@" 0) 1) 0)) (>= (- (str.len in0) (+ (str.indexof in0 "@" 0) 1)) 0)))

(check-sat)

;(get-value (in0))





