export function always<T>(a: T): (...args: any[]) => T {
    return (...args: any[]) => a;
}

export function function_compose<A, B, C>(f: (b: B) => C, g: (a: A) => B): (a: A) => C {
    return (a: A) => f(g(a));
}

export function function_power<T>(f: (x: T) => T, n: number): (x: T) => T {
    return function_power_v2(f, n)
}

export function identity<T>(x: T): T {
    return x;
}

export function equals<T>(a: T, b: T): boolean {
    return a === b;
}

export function function_power_v1<T>(f: (x: T) => T, n: number): (x: T) => T {
    // This may cause stack overflow.
    let power = (x: T) => x
    for (let i = 0; i < n; i++) {
        power = function_compose(power, f)
    }
    return power
}

export function function_power_v2<T>(f: (x: T) => T, n: number): (x: T) => T {
    if (n == 0) {
        return identity
    }
    return (x: T) => {
        let result = x
        for (let i = 0; i < n; i++) {
            result = f(result)
            // console.log(result)
        }
        return result
    }
}
