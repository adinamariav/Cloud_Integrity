import React from "react";
import SandboxCard from "../includes/SandboxingCard.js";
import AnalyzeCard from "../includes/AnalyzeCard.js";
import EducationalCard from "../includes/EducationalCard.js";

function Home() {
    return(
        <div className="container">
            <div className="card mt-4" style = {{ backgroundColor: `#F4F9F9`, paddingLeft: `10rem`, paddingRight: `10rem`}}>
                <div className="card-body">
                    <h1 style = {{ textAlign: `center` }}>Please select an operating mode:</h1>
                    <div style={{paddingTop: `2rem`}}>
                        <AnalyzeCard />
                        <SandboxCard />
                        <EducationalCard />
                    </div>
                    
                </div>
            </div>
        </div>
    );
}

export default Home;