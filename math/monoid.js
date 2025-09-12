export function generate_monoid(generators, multiply, eq, limit = 200) {
    let ret = Array.from(generators);
    let last_length = 0;
    let current_length = ret.length;
    console.log("generating elements of the monoid, size from " + last_length + " to " + current_length);
    while (last_length < current_length) {
        let step = 20;
        for (let i = 0; i < current_length; i++) {
            if (i % Math.max(1, Math.floor(current_length / step)) == 0) {
                console.debug("working on " + i + "/" + current_length);
            }
            for (let j = last_length; j < current_length; j++) {
                let product_ij = multiply(ret[i], ret[j]);
                if (!ret.some(ele => eq(ele, product_ij))) {
                    console.log("adding " + i + ":" + ret[i] + " " + j + ":" + ret[j] + " " + product_ij);
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
        console.log("generating elements of the monoid, size from " + last_length + " to " + current_length);
        if (limit > 0 && current_length > limit) {
            break;
        }
    }
    return ret;
}
