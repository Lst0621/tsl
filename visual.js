export function draw_multiplication_table(table, input, multiply, to_string, input_get_color, product_get_color) {
    while (true) {
        if (table.rows.length == 0) {
            break;
        }
        table.deleteRow(0);
    }
    table.style.alignSelf = "center";
    table.style.borderStyle = "solid";
    table.style.textAlign = "center";
    {
        let row = table.insertRow();
        row.insertCell();
        for (let i = 0; i < input.length; i++) {
            let cell = row.insertCell();
            cell.style.borderStyle = "solid";
            cell.innerHTML = to_string(input[i]);
            cell.style.background = input_get_color(input[i]);
        }
    }
    for (let i = 0; i < input.length; i++) {
        let row = table.insertRow();
        let cell = row.insertCell();
        cell.style.borderStyle = "solid";
        cell.style.background = input_get_color(input[i]);
        cell.innerHTML = to_string(input[i]);
        for (let j = 0; j < input.length; j++) {
            let cell_product = row.insertCell();
            cell_product.style.borderStyle = "solid";
            let product = multiply(input[i], input[j]);
            cell_product.innerHTML = to_string(product);
            cell_product.style.background = product_get_color(input[i], input[j], product);
        }
    }
}
