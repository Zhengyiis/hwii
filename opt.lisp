(if (< 2 (+ 2 (+ 4 (add1 5))))
    5
    (let ((x (+ 4 5)))
     (+ x x)))

(+ (read-num) (+ 2 2))



(define (f x)
    (+ x 2))
(print (* (f (read-num)) (f (read-num))))

















(define (g x)
    (* x 3))
(define (f x)
    (+ (g x) (g x)))
(print (* (f (read-num)) (f (read-num))))


















(define (map f l)
    (if (empty? l) ())
        (pair (f (left l)) (map f (right l))))
(define (f x)
    (+ (g x) 2))
(print (map f (pair 1 (pair 2 ())))) 








(let ((x (read-num)))  
    (+ (* (+ x 2) (+ x 2)) (+ x 2)))















(define (sum-to x)  
    (if (= x 0) 0    
        (+ x (sum-to (sub1 x)))))
(let ((x (read-num)))
    (+ (* (sum-to x) (sum-to x)) (sum-to x)))














(define (read-plus x)
    (+ (read-num) x))
(let ((x (read-num)))
    (+ (* (read-plus x) (read-plus x)) (read-plus x))) 