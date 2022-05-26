import React from "react";
import SelectVM from "../includes/SelectVM.js"
import Learn from "../includes/Learn.js"
import Analyze from "../includes/Analyze.js"

function Home() {
    return(
        <div className="container">
            <div className="card mt-4" style = {{ backgroundColor: `lightblue`}}>
                <div className="row card-body">

                    <div className="col">
                        <SelectVM />
                    </div>
                    
                    <div className="col">
                        <Analyze />
                        <Learn />
                    </div>
                    
                </div>
            </div>
        </div>
    );
}

export default Home;