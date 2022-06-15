import React from "react";
import { Link } from 'react-router-dom'

function SandboxCard() {
        return(
            <div class= "card shadow" style= {{ display: `inline-block`, backgroundColor: `#D3E0EA`, width: `100%`, textAlign: `center`, marginBottom: `2rem` }}>
                <div class="card-body">
                    <h2 class="card-title">Sandboxing</h2>
                    <p class="card-text">Some quick example text to build on the card title and make up the bulk of the card's content.</p>
                    <p class="card-text">Some quick example text to build on the card title and make up the bulk of the card's content.</p>
                    <p class="card-text">Some quick example text to build on the card title and make up the bulk of the card's content.</p>
                    <Link to="/sandbox" class="stretched-link"></Link>
                </div>
            </div>
        );
}

export default SandboxCard;