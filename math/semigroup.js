import { cartesian_product } from "./set.js";
export function generate_semigroup(generators, multiply, eq, limit = -1) {
    let ret = Array.from(generators);
    let last_length = 0;
    let current_length = ret.length;
    console.debug("generating elements of the semigroup, size from " + last_length + " to " + current_length);
    while (last_length < current_length) {
        let step = 20;
        for (let i = 0; i < current_length; i++) {
            if (i % Math.max(1, Math.floor(current_length / step)) == 0) {
                console.debug("working on " + i + "/" + current_length);
            }
            for (let j = last_length; j < current_length; j++) {
                let product_ij = multiply(ret[i], ret[j]);
                if (!ret.some(ele => eq(ele, product_ij))) {
                    console.debug("adding " + i + ":" + ret[i] + " " + j + ":" + ret[j] + " " + product_ij);
                    ret.push(product_ij);
                }
            }
        }
        for (let i = 0; i < current_length; i++) {
            for (let j = last_length; j < current_length; j++) {
                let product_ji = multiply(ret[j], ret[i]);
                if (!ret.some(ele => eq(ele, product_ji))) {
                    console.log("adding " + j + ":" + ret[j] + " " + i + ":" + ret[i] + " " + product_ji);
                    ret.push(product_ji);
                }
            }
        }
        last_length = current_length;
        current_length = ret.length;
        console.debug("generating elements of the semigroup, size from " + last_length + " to " + current_length);
        if (limit > 0 && current_length > limit) {
            break;
        }
    }
    return ret;
}
export function is_closure(generators, multiply, eq) {
    let limit = generators.length + 1;
    // no need to generate the whole semigroup, just check if it is already closed
    let closure = generate_semigroup(generators, multiply, eq, limit);
    return closure.length == closure.length;
}
export function get_idempotent_power(item, multiply, eq, limit = -1) {
    let square = multiply(item, item);
    let power_t = item;
    let power_2t = square;
    for (let i = 1; i != limit; i++) {
        if (eq(power_t, power_2t)) {
            return [i, power_t];
        }
        power_t = multiply(power_t, item);
        power_2t = multiply(power_2t, square);
    }
    return [-1, null];
}
export function get_all_idempotent_elements(elements, multiply, eq) {
    return elements.filter(item => eq(multiply(item, item), item));
}
export function get_highest_idempotent_power(elements, multiply, eq) {
    return Math.max(...elements.map(item => get_idempotent_power(item, multiply, eq)[0]));
}
export function is_abelian(elements, multiply, eq) {
    let len = elements.length;
    for (let i = 0; i < len; i++) {
        for (let j = 0; j < i; j++) {
            let ab = multiply(elements[i], elements[j]);
            let ba = multiply(elements[j], elements[i]);
            if (!eq(ab, ba)) {
                return false;
            }
        }
    }
    return true;
}
export function semigroup_power(base, exponent, multiply) {
    if (exponent == 1) {
        return base;
    }
    let half = Math.floor(exponent / 2);
    let half_power = semigroup_power(base, half, multiply);
    if (exponent % 2 == 0) {
        return multiply(half_power, half_power);
    }
    else {
        return multiply(multiply(half_power, half_power), base);
    }
}
function get_definite_k_common(elements, multiply, eq, multiply_idempotent_on_right) {
    let highest_idempotent_power = get_highest_idempotent_power(elements, multiply, eq);
    let candidates = elements.map(item => semigroup_power(item, highest_idempotent_power, multiply));
    for (let pair of cartesian_product([elements, candidates])) {
        let element = pair[0];
        let candidate = pair[1];
        let product = multiply_idempotent_on_right ? multiply(element, candidate) : multiply(candidate, element);
        if (!eq(product, candidate)) {
            return -1;
        }
    }
    return highest_idempotent_power;
}
export function get_definite_k(elements, multiply, eq) {
    return get_definite_k_common(elements, multiply, eq, true);
}
export function get_reverse_definite_k(elements, multiply, eq) {
    return get_definite_k_common(elements, multiply, eq, false);
}
export function is_aperiodic(elements, multiply, eq) {
    let max_power = 1;
    for (let element of elements) {
        let [power, idempotent_element] = get_idempotent_power(element, multiply, eq);
        if (idempotent_element == null) {
            console.log("Idempotent power is not available");
            return -1;
        }
        if (!eq(multiply(idempotent_element, element), idempotent_element)) {
            console.log("Element does not satisfy aperiodic condition");
            return -1;
        }
        max_power = Math.max(max_power, power);
    }
    return max_power;
}
export function is_monoid(elements, multiply, eq) {
    if (!is_closure(elements, multiply, eq)) {
        console.log("Not even a semigroup");
        return [false, null];
    }
    let idempotent_elements = get_all_idempotent_elements(elements, multiply, eq);
    console.log(idempotent_elements.length);
    for (let idempotent of idempotent_elements) {
        let is_identity = true;
        // find an element e, such that for all a in S, e*a = a*e = a
        for (let element of elements) {
            if (!eq(multiply(idempotent, element), element) || !eq(multiply(element, idempotent), element)) {
                is_identity = false;
                break;
            }
        }
        if (is_identity) {
            return [true, idempotent];
        }
    }
    console.log("No identity element found");
    return [false, null];
}
export function is_group(elements, multiply, eq) {
    let [is_monoid_result, optional_identity_element] = is_monoid(elements, multiply, eq);
    if (!is_monoid_result) {
        console.log("Not a monoid, so not a group");
        return [false, null];
    }
    let identity_element = optional_identity_element;
    for (let element of elements) {
        let has_inverse = false;
        for (let other_element of elements) {
            let product = multiply(element, other_element);
            if (eq(product, element)) {
                // in a group, ab=e then ba=e
                has_inverse = eq(multiply(other_element, element), identity_element);
                break;
            }
        }
        if (!has_inverse) {
            return [false, null];
        }
    }
    return [true, identity_element];
}
