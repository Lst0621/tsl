export function always(a) {
    return (...args) => a;
}
export function function_compose(f, g) {
    return (a) => f(g(a));
}
export function function_power(f, n) {
    return function_power_v2(f, n);
}
export function identity(x) {
    return x;
}
export function function_power_v1(f, n) {
    // This may cause stack overflow.
    let power = (x) => x;
    for (let i = 0; i < n; i++) {
        power = function_compose(power, f);
    }
    return power;
}
export function function_power_v2(f, n) {
    if (n == 0) {
        return identity;
    }
    return (x) => {
        let result = x;
        for (let i = 0; i < n; i++) {
            result = f(result);
            // console.log(result)
        }
        return result;
    };
}
