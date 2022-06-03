import React from "react";
import CustomListDropDown from "./Dropdown";

function Select() {
    return(
        <div className="container">
            <div>
                <div className="card mt-4 row shadow border-info align-items-start" style= {{ width: `18rem`, backgroundColor: `lavender` }}>
                    <div className="card-body col text-center">
                        <img src= {require("../images/vm.png")} class="img-fluid" alt="..." />
                    </div>
                    <CustomListDropDown />
                </div>
            </div>
        </div>
    );
}

export default Select;