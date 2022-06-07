import React, { Component } from "react";
import CustomListDropDown from "./Dropdown";
import { DataContext } from "./Context";

class Select extends Component {
    
    static contextType = DataContext;

    render () {
        return(
            <div className="container">
                <div>
                    {/* <div className="card mt-4 row shadowalign-items-start" style= {{ width: `18rem`, backgroundColor: `silver`, paddingBottom: `2rem` }}>
                        <div className="card-body col text-center">
                            <img src= {require("../images/vm.png")} class="img-fluid" alt="..." />
                        </div>
                        <CustomListDropDown />
                        <button type="button" onClick={() => this.context.startVM()} class="btn btn-danger shadow" style= {{ marginTop: `2rem`, width: `30%`, marginLeft: `6rem`}}>Start</button>
                    </div> */}
                    <iframe src = 'http://127.0.0.1:5001/remote/ssh/adina/22' width = "900" height = "600" />
                </div>
            </div>
        );
    }
}

export default Select;