export function generate_semigroup<T>(generators: T[],
                                      multiply: (a: T, b: T) => T,
                                      eq: (a: T, b: T) => boolean,
                                      limit: number = 200): T[] {
    let ret: T[] = Array.from(generators)
    let last_length: number = 0
    let current_length: number = ret.length
    console.debug("generating elements of the semigroup, size from " + last_length + " to " + current_length)
    while (last_length < current_length) {
        let step = 20
        for (let i = 0; i < current_length; i++) {
            if (i % Math.max(1, Math.floor(current_length / step)) == 0) {
                console.debug("working on " + i + "/" + current_length)
            }
            for (let j = last_length; j < current_length; j++) {
                let product_ij = multiply(ret[i], ret[j])
                if (!ret.some(ele => eq(ele, product_ij))) {
                    console.debug("adding " + i + ":" + ret[i] + " " + j + ":" + ret[j] + " " + product_ij)
                    ret.push(product_ij)
                }
            }
        }

        for (let i = 0; i < current_length; i++) {
            for (let j = last_length; j < current_length; j++) {
                let product_ji = multiply(ret[j], ret[i])
                if (!ret.some(ele => eq(ele, product_ji))) {
                    console.log("adding " + j + ":" + ret[j] + " " + i + ":" + ret[i] + " " + product_ji)
                    ret.push(product_ji)
                }
            }
        }

        last_length = current_length
        current_length = ret.length
        console.debug("generating elements of the semigroup, size from " + last_length + " to " + current_length)
        if (limit > 0 && current_length > limit) {
            break;
        }
    }

    return ret
}

export function get_idempotent_power<T>(item: T,
                                        multiply: (a: T, b: T) => T,
                                        eq: (a: T, b: T) => boolean,
                                        limit: number = -1): [number, T | null] {
    let square = multiply(item, item)
    let power_t: T = item
    let power_2t: T = square
    for (let i = 1; i != limit; i++) {
        if (eq(power_t, power_2t)) {
            return [i, power_t]
        }
        power_t = multiply(power_t, item)
        power_2t = multiply(power_2t, square)
    }
    return [-1, null]
}